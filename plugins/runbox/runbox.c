#include <stdlib.h>
#include <string.h>

#include <glib.h>

#define USE_DBUS
#ifdef USE_DBUS
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "gtkbgbox.h"

//#define DEBUGPRN
#include "dbg.h"

enum runbox_modes {RB_HIDDEN, RB_ICON, RB_FULL};

typedef struct {
    plugin_instance plugin;
    GtkWidget *combobox_textentry;
    GtkWidget *combobox;
    GtkWidget *subwin;
    GtkWidget *base;
    int history_size;
    int history_maxsize;
    char **history;
    gchar *historyfile;
    int size;
    int hspaces;
    enum runbox_modes mode;
    GMainLoop *loop;
#ifdef USE_DBUS
    DBusConnection *bus;
#endif
} runbox_priv;

static void read_history_from_file(runbox_priv *rb) {
    FILE *f;
    int len;
    char buf[1024];

    ENTER;

    DBG("Read file %s\n", rb->historyfile);
    f = fopen(rb->historyfile, "r");
    rb->history_size = 0;
    if (f) {
        while( fgets(buf, 1024, f) ) {
            DBG("Line: %s\n", buf);

            len=strlen(buf);
            DBG("Line lenght = %d\n", len);
            if (len > 1022) {
                ERR("Line too long!\n");
                fclose(f);
                RET();
            }
            if (len > 0) {
                buf[len-1] = '\0';
                rb->history[rb->history_size] = strdup(buf);
                rb->history_size++;
                if (rb->history_size == rb->history_maxsize)
                    break;
            }
        }
        fclose(f);
    }

    RET();
}
static void write_history_to_file(runbox_priv *rb) {
    FILE *f;
    int i;

    ENTER;

    f = fopen(rb->historyfile, "w");
    if (f == NULL)
        RET();

    for (i = 0; i < rb->history_size; i++) {
        fprintf(f, "%s\n", rb->history[i]);
    }
    fclose(f);

    RET();
}

static int add_to_history(const char *text, runbox_priv *rb) {
    int i;
    char *buf, *tmp;

    ENTER;

    if (rb->history[0] != NULL) {
        if (strcmp(text, rb->history[0]) == 0 )
            RET(0);
    } else {
        rb->history_size = 1;
    }

    buf = rb->history[0];
    rb->history[0] = strdup(text);

    for (i = 1; i < rb->history_size; i++) {
        DBG("move %d (%s) -> %d\n", i-1, buf, i);
        tmp = rb->history[i];
        rb->history[i] = buf;

        if(strcmp(text, tmp) == 0) {
            free(tmp);
            buf = NULL;
            break;
        } else {
            buf = tmp;
        }
    }
    if (buf) {
        if (rb->history_size < rb->history_maxsize) {
            DBG("Add new entry %d (%s) to history\n", rb->history_size, buf);
            rb->history[rb->history_size] = buf;
            rb->history_size++;
        } else {
            free(buf);
        }
    }

    RET(1);
}

static void runbox_comboentry_changed(runbox_priv *rb) {
    gtk_widget_grab_focus(rb->combobox_textentry);
    gtk_editable_set_position((GtkEditable*)rb->combobox_textentry, -1);
}

static void runbox_comboentry_create(runbox_priv *rb) {
    GtkEntryCompletion *comp;
    GtkListStore *m;
    GtkTreeIter iter;
    int i;

    ENTER;

    rb->combobox = gtk_combo_box_text_new_with_entry();
    g_signal_connect_swapped(G_OBJECT(rb->combobox),
        "changed", (GCallback)runbox_comboentry_changed, rb);
    rb->combobox_textentry = gtk_bin_get_child (GTK_BIN (rb->combobox));

    comp = gtk_entry_completion_new();
    gtk_entry_completion_set_text_column(comp, 0);
    m = gtk_list_store_new(1, G_TYPE_STRING);
    gtk_entry_completion_set_model(comp, GTK_TREE_MODEL(m));
    g_object_unref(m);

    for (i = 0; i < rb->history_size; i++) {
        gtk_list_store_append(m, &iter);
        gtk_list_store_set(m, &iter, 0, rb->history[i], -1);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rb->combobox), rb->history[i]);
    }

    gtk_entry_set_completion(GTK_ENTRY(rb->combobox_textentry), comp);

    RET();
}

static int run_app2(gchar *cmd)
{
    GError *error = NULL;

    ENTER;
    if (!cmd)
        RET(-1);

    if (!g_spawn_command_line_async(cmd, &error))
    {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, 0, 
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "%s", error->message);
        gtk_widget_show_all(dialog);
        gtk_window_present_with_time(GTK_WINDOW(dialog), time(NULL));
        g_signal_connect_swapped (dialog,
                                  "response",
                                  G_CALLBACK (gtk_widget_destroy),
                                  dialog);
        g_error_free(error);
        RET(-2);
    }
    RET(0);
}

static void runbox_comboentry_exec(runbox_priv *rb, char *cmd) {
    ENTER;

    DBG("cmd = %s\n", cmd);

    if ( ! run_app2(cmd) ) {
        if ( add_to_history(cmd, rb) && rb->historyfile ) {
            write_history_to_file(rb);
        }
    }

    RET();
}



//
// dialog widget
//

static void runbox_combo_entry_dialog_response (GtkDialog *dialog, gint code, runbox_priv *rb) {
    gchar *cmd = NULL;
    ENTER;

    DBG("code = %d\n", code);
    if (code == GTK_RESPONSE_OK) {
        cmd = g_strdup(gtk_entry_get_text(GTK_ENTRY(rb->combobox_textentry)));
    }

    gtk_widget_destroy(GTK_WIDGET(rb->subwin));
    rb->subwin = NULL;

    if (cmd) {
        runbox_comboentry_exec(rb, cmd);
        g_free(cmd);
    }

    RET();
}

static GtkResponseType runbox_combo_entry_dialog_show(runbox_priv *rb)
{
    ENTER;

    rb->subwin = gtk_dialog_new_with_buttons ("Run command ...", NULL,
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_STOCK_OK,      GTK_RESPONSE_OK,
                                           GTK_STOCK_CANCEL,  GTK_RESPONSE_CANCEL,
                                           NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(rb->subwin), GTK_RESPONSE_OK);

    runbox_comboentry_create(rb);
    gtk_entry_set_activates_default(GTK_ENTRY(rb->combobox_textentry), TRUE);

    gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(rb->subwin))),
        GTK_WIDGET(rb->combobox), TRUE, TRUE, 0);
    gtk_widget_show (GTK_WIDGET(rb->combobox));

    g_signal_connect (rb->subwin,
                      "response",
                      G_CALLBACK (runbox_combo_entry_dialog_response),
                      rb);
    gtk_window_set_modal(GTK_WINDOW(rb->subwin), FALSE);
    gtk_widget_show_all(rb->subwin);

    RET(0);
}




//
// widget for panel
//

static gboolean
my_button_pressed(GtkWidget *widget, GdkEventButton *event, runbox_priv *rb)
{
    ENTER;
    /* propagate Control-Button3 to the panel */
    if (event->type == GDK_BUTTON_PRESS && event->button == 3
        && event->state & GDK_CONTROL_MASK)
    {
        RET(FALSE);
    }

    if ((event->type == GDK_BUTTON_PRESS)
        && (event->x >=0 && event->x < widget->allocation.width)
        && (event->y >=0 && event->y < widget->allocation.height))
    {
        if (!rb->subwin)
            runbox_combo_entry_dialog_show(rb);
    }
    RET(TRUE);
}

static GtkWidget *make_button(runbox_priv *rb) {
    int w, h;
    gchar *fname, *iname;
    GtkWidget *bg = NULL;

    ENTER;

    /* XXX: this code is duplicated in every plugin.
     * Lets run it once in a panel */
    if (rb->plugin.panel->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        w = -1;
        h = rb->plugin.panel->max_elem_height;
    }
    else
    {
        w = rb->plugin.panel->max_elem_height;
        h = -1;
    }
    fname = iname = NULL;
    XCG(rb->plugin.xc, "image", &fname, str);
    fname = expand_tilda(fname);
    XCG(rb->plugin.xc, "icon", &iname, str);
    if (fname || iname)
    {
        bg = fb_button_new(iname, fname, w, h, 0x282828, NULL);
        gtk_container_add(GTK_CONTAINER(rb->plugin.pwid), bg);
        if (rb->plugin.panel->transparent)
            gtk_bgbox_set_background(bg, BG_INHERIT, 0, 0);
        g_signal_connect (G_OBJECT (bg), "button-press-event",
            G_CALLBACK (my_button_pressed), rb);
    }
    g_free(fname);

    RET(bg);
}

static GtkWidget *make_entrybox(runbox_priv *rb) {
    GtkWidget *basebox, *topspacebox, *bottomspacebox;

    ENTER;

    if (rb->plugin.panel->orientation == GTK_ORIENTATION_HORIZONTAL) {
        basebox = gtk_vbox_new(TRUE, 1);
        gtk_container_set_border_width (GTK_CONTAINER (basebox), 0);
        gtk_box_set_homogeneous(GTK_BOX(basebox), FALSE);
        gtk_widget_show(basebox);
        gtk_container_add(GTK_CONTAINER(rb->plugin.pwid), basebox);

        if (rb->hspaces != 0) {
            topspacebox = gtk_hbox_new(TRUE, 1);
            gtk_widget_set_size_request(topspacebox, rb->size, rb->hspaces);
            gtk_widget_show(topspacebox);
            gtk_container_add(GTK_CONTAINER(basebox), topspacebox);
        }
    } else {
        basebox = rb->plugin.pwid;
    }

    runbox_comboentry_create(rb);
    gtk_container_add(GTK_CONTAINER(basebox), rb->combobox);
    gtk_widget_show (GTK_WIDGET(rb->combobox));

    if (rb->plugin.panel->orientation == GTK_ORIENTATION_HORIZONTAL && rb->hspaces != 0) {
        bottomspacebox = gtk_hbox_new(TRUE, 1);
        gtk_widget_show(bottomspacebox);
        gtk_widget_set_size_request(bottomspacebox, rb->size, rb->hspaces);
        gtk_container_add(GTK_CONTAINER(basebox), bottomspacebox);
    }

    RET( basebox );
}


#ifdef USE_DBUS

//
// dbus signals
//
//  based on example code from:
//  http://www.ibm.com/developerworks/linux/library/l-dbus/index.html
//

static DBusHandlerResult
signal_filter (DBusConnection *connection, DBusMessage *message, runbox_priv *rb) {
    ENTER;

    if (dbus_message_is_signal 
        (message, "org.freedesktop.Local", "Disconnected")) {
        g_main_loop_quit (rb->loop);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal (message, "net.sourceforge.fbpanel.Signal", "RunCmd")) {
        DBusError error;
        char *s;
        dbus_error_init (&error);
        if (dbus_message_get_args 
           (message, &error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID)) {
           if (rb->mode == RB_FULL) { // FIXME
               gtk_window_present_with_time(GTK_WINDOW(rb->plugin.panel->topgwin), time(NULL));
               //gdk_window_focus(GTK_WINDOW(rb->plugin.panel->topgwin->window), time(NULL));
               gtk_entry_set_text(GTK_ENTRY(rb->combobox_textentry), s);
               runbox_comboentry_changed(rb);
           } else {
               runbox_combo_entry_dialog_show(rb);
               gtk_entry_set_text(GTK_ENTRY(rb->combobox_textentry), s);
               runbox_comboentry_changed(rb);
               if (rb->subwin) {
                   gtk_window_present_with_time(GTK_WINDOW(rb->subwin), time(NULL));
               }
           }
        } else {
            ERR("Dbus signal received error: %s\n", error.message);
            dbus_error_free (&error);
        }
      RET( DBUS_HANDLER_RESULT_HANDLED );
    }
    RET( DBUS_HANDLER_RESULT_NOT_YET_HANDLED );
}

static void init_dbus(runbox_priv *rb) {
    DBusError error;

    ENTER;

    dbus_error_init (&error);
    rb->bus = dbus_bus_get (DBUS_BUS_SESSION, &error);
    if (!rb->bus) {
        ERR("Failed to connect to the D-BUS daemon: %s", error.message);
        dbus_error_free (&error);
    }
    dbus_connection_setup_with_g_main (rb->bus, NULL);

    dbus_bus_add_match (rb->bus, "type='signal',interface='net.sourceforge.fbpanel.Signal'", NULL);

    rb->loop = g_main_loop_new (NULL, FALSE);
    dbus_connection_add_filter (rb->bus, (DBusHandleMessageFunction)signal_filter, rb, NULL);

    RET();
}

#endif

//
// plugin base functions
//

static int
runbox_constructor(plugin_instance *p)
{
    gchar *buf;

    ENTER;

    runbox_priv *rb = (runbox_priv *) p;

    XCG(p->xc, "show", &buf, str);
    rb->size = 150;
    XCG(p->xc, "size", &rb->size, int);
    rb->hspaces = 17;
    XCG(p->xc, "HorizontalSpace", &rb->hspaces, int);
    rb->history_maxsize = 20;
    XCG(p->xc, "HistorySize", &rb->history_maxsize, int);
    rb->history = calloc(rb->history_maxsize, sizeof(char*));
    rb->history[0] = NULL;
    rb->history_size = 0;

    rb->historyfile = NULL;
    XCG(p->xc, "HistoryFile", &rb->historyfile, str);
    rb->historyfile = expand_tilda(rb->historyfile);
    if (rb->historyfile)
        read_history_from_file(rb);

    rb->base = NULL;
    rb->subwin = NULL;
    rb->combobox = NULL;

    if (!strcmp(buf, "icon")) {
        DBG("Create iconon panel\n");
        rb->mode = RB_ICON;
        rb->base = make_button(rb);
    } else if (!strcmp(buf, "full")) {
        DBG("Puts combo entry box on panel\n");
        rb->mode = RB_FULL;

        gtk_window_set_accept_focus(GTK_WINDOW(rb->plugin.panel->topgwin), TRUE);
/* FIXME GDK_WINDOW_TYPE_HINT_DOCK dont accept focus?
        gtk_window_set_type_hint(GTK_WINDOW(rb->plugin.panel->topgwin), GDK_WINDOW_TYPE_HINT_TOOLBAR);

        Atom dock[5];
        dock[0] = XInternAtom(GDK_DISPLAY(),"_NET_WM_WINDOW_TYPE_DOCK", False);
        XChangeProperty(GDK_DISPLAY(), rb->plugin.panel->topxwin, a_NET_WM_WINDOW_TYPE,
            XA_ATOM, 32, PropModeAppend, (unsigned char *) dock, 1);
        XMapWindow(GDK_DISPLAY(), rb->plugin.panel->topxwin);
        XSync(GDK_DISPLAY(), False);

        dock[0] = XInternAtom(GDK_DISPLAY(),"_NET_WM_STATE_SKIP_TASKBAR", False);
        dock[1] = XInternAtom(GDK_DISPLAY(),"_NET_WM_STATE_SKIP_PAGER", False);
        dock[2] = XInternAtom(GDK_DISPLAY(),"_NET_WM_STATE_STICKY", False);
        dock[3] = XInternAtom(GDK_DISPLAY(),"_NET_WM_STATE_STAYS_ON_TOP", False);
        dock[4] = XInternAtom(GDK_DISPLAY(),"_NET_WM_STATE_ABOVE", False);
        XChangeProperty(GDK_DISPLAY(), rb->plugin.panel->topxwin, a_NET_WM_STATE,
            XA_ATOM, 32, PropModeAppend, (unsigned char *) dock, 5);

        XMapWindow(GDK_DISPLAY(), rb->plugin.panel->topxwin);
        XSync(GDK_DISPLAY(), False);
*/

        rb->base = make_entrybox(rb);
    } else {
        DBG("Start in HIDDEN mode\n");
        rb->mode = RB_HIDDEN;
    }

#ifdef USE_DBUS
    init_dbus(rb);
#endif

    RET(1);
}

static void
runbox_destructor(plugin_instance *p)
{
    ENTER;

    runbox_priv *rb = (runbox_priv *) p;
    g_free(rb->historyfile);
    free(rb->history);

    RET();
}

static plugin_class class = {
    .fname       = NULL,
    .count       = 0,
    .type        = "runbox",
    .name        = "runbox",
    .version     = "1.0",
    .description = "runbox in a panel",
    .priv_size   = sizeof(runbox_priv),

    .constructor = runbox_constructor,
    .destructor  = runbox_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
