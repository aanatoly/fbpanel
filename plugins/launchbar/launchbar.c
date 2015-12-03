
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>


#include <gdk-pixbuf/gdk-pixbuf.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "gtkbgbox.h"
#include "run.h"
#include "gtkbar.h"

//#define DEBUGPRN
#include "dbg.h"


typedef enum {
    CURSOR_STANDARD,
    CURSOR_DND
} CursorType;

enum {
    TARGET_URILIST,
    TARGET_MOZ_URL,
    TARGET_UTF8_STRING,
    TARGET_STRING,
    TARGET_TEXT,
    TARGET_COMPOUND_TEXT
};

static const GtkTargetEntry target_table[] = {
    { "text/uri-list", 0, TARGET_URILIST},
    { "text/x-moz-url", 0, TARGET_MOZ_URL},
    { "UTF8_STRING", 0, TARGET_UTF8_STRING },
    { "COMPOUND_TEXT", 0, 0 },
    { "TEXT",          0, 0 },
    { "STRING",        0, 0 }
};

struct launchbarb;

typedef struct btn {
    //GtkWidget *button, *pixmap;
    struct launchbar_priv *lb;
    gchar *action;
} btn;

#define MAXBUTTONS 20
typedef struct launchbar_priv {
    plugin_instance plugin;
    GtkWidget *box;
    btn btns[MAXBUTTONS];
    int btn_num;
    int iconsize;
    unsigned int discard_release_event : 1;
} launchbar_priv;


static gboolean
my_button_pressed(GtkWidget *widget, GdkEventButton *event, btn *b )
{
    ENTER;
    if (event->type == GDK_BUTTON_PRESS && event->button == 3
        && event->state & GDK_CONTROL_MASK)
    {
        b->lb->discard_release_event = 1;
        RET(FALSE);
    }
    if (event->type == GDK_BUTTON_RELEASE && b->lb->discard_release_event)
    {
        b->lb->discard_release_event = 0;
        RET(TRUE);
    }
    g_assert(b != NULL);
    if (event->type == GDK_BUTTON_RELEASE)
    {
        if ((event->x >=0 && event->x < widget->allocation.width)
            && (event->y >=0 && event->y < widget->allocation.height))
        {
            run_app(b->action);
        }
    }
    RET(TRUE);
}

static void
launchbar_destructor(plugin_instance *p)
{
    launchbar_priv *lb = (launchbar_priv *) p;
    int i;

    ENTER;
    gtk_widget_destroy(lb->box);
    for (i = 0; i < lb->btn_num; i++) 
        g_free(lb->btns[i].action);     

    RET();
}


static void
drag_data_received_cb (GtkWidget *widget,
    GdkDragContext *context,
    gint x,
    gint y,
    GtkSelectionData *sd,
    guint info,
    guint time,
    btn *b)
{
    gchar *s, *str, *tmp, *tok, *tok2;

    ENTER;
    if (sd->length <= 0)
        RET();
    DBG("uri drag received: info=%d/%s len=%d data=%s\n",
         info, target_table[info].target, sd->length, sd->data);
    if (info == TARGET_URILIST)
    {
        /* white-space separated list of uri's */
        s = g_strdup((gchar *)sd->data);
        str = g_strdup(b->action);
        for (tok = strtok(s, "\n \t\r"); tok; tok = strtok(NULL, "\n \t\r"))
        {
            tok2 = g_filename_from_uri(tok, NULL, NULL);
            /* if conversion to filename was ok, use it, otherwise
             * lets append original uri */
            tmp = g_strdup_printf("%s '%s'", str, tok2 ? tok2 : tok);
            g_free(str);
            g_free(tok2);
            str = tmp;
        }
        DBG("cmd=<%s>\n", str);
        g_spawn_command_line_async(str, NULL);
        g_free(str);
        g_free(s);
    }
    else if (info == TARGET_MOZ_URL)
    {
        gchar *utf8, *tmp;
        
	utf8 = g_utf16_to_utf8((gunichar2 *) sd->data, (glong) sd->length,
              NULL, NULL, NULL);
        tmp = utf8 ? strchr(utf8, '\n') : NULL;
	if (!tmp)
        {
            ERR("Invalid UTF16 from text/x-moz-url target");
            g_free(utf8);
            gtk_drag_finish(context, FALSE, FALSE, time);
            RET();
	}
	*tmp = '\0';
        tmp = g_strdup_printf("%s %s", b->action, utf8);
        g_spawn_command_line_async(tmp, NULL);
        DBG("%s %s\n", b->action, utf8);
        g_free(utf8);
        g_free(tmp);
    }
    RET();
}

static int
read_button(plugin_instance *p, xconf *xc)
{
    launchbar_priv *lb = (launchbar_priv *) p;
    gchar *iname, *fname, *tooltip, *action;
    GtkWidget *button;
    
    ENTER;
    if (lb->btn_num >= MAXBUTTONS)
    {
        ERR("launchbar: max number of buttons (%d) was reached."
            "skipping the rest\n", lb->btn_num );
        RET(0);
    }
    iname = tooltip = fname = action = NULL;
    XCG(xc, "image", &fname, str);
    XCG(xc, "icon",  &iname, str);
    XCG(xc, "action", &action, str);
    XCG(xc, "tooltip", &tooltip, str);

    action = expand_tilda(action);
    fname = expand_tilda(fname);

    button = fb_button_new(iname, fname, lb->iconsize,
        lb->iconsize, 0x202020, NULL);

    g_signal_connect (G_OBJECT (button), "button-release-event",
          G_CALLBACK (my_button_pressed), (gpointer) &lb->btns[lb->btn_num]);
    g_signal_connect (G_OBJECT (button), "button-press-event",
          G_CALLBACK (my_button_pressed), (gpointer) &lb->btns[lb->btn_num]);

    GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
    // DnD support
    gtk_drag_dest_set (GTK_WIDGET(button),
        GTK_DEST_DEFAULT_ALL, //GTK_DEST_DEFAULT_HIGHLIGHT,
        target_table, G_N_ELEMENTS (target_table),
        GDK_ACTION_COPY);    
    g_signal_connect (G_OBJECT(button), "drag_data_received",
        G_CALLBACK (drag_data_received_cb),
        (gpointer) &lb->btns[lb->btn_num]);

    gtk_box_pack_start(GTK_BOX(lb->box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    if (p->panel->transparent) 
        gtk_bgbox_set_background(button, BG_INHERIT,
            p->panel->tintcolor, p->panel->alpha);
    gtk_widget_set_tooltip_markup(button, tooltip);
    
    g_free(fname);
    //g_free(iname);
    DBG("here\n");

    lb->btns[lb->btn_num].action = action;
    lb->btns[lb->btn_num].lb     = lb;
    lb->btn_num++;
    
    RET(1);
}

static void
launchbar_size_alloc(GtkWidget *widget, GtkAllocation *a,
    launchbar_priv *lb)
{
    int dim;

    ENTER;
    if (lb->plugin.panel->orientation == GTK_ORIENTATION_HORIZONTAL) 
        dim = a->height / lb->iconsize;
    else
        dim = a->width / lb->iconsize;
    DBG("width=%d height=%d iconsize=%d -> dim=%d\n",
        a->width, a->height, lb->iconsize, dim);
    gtk_bar_set_dimension(GTK_BAR(lb->box), dim);
    RET();
}
                                      
static int
launchbar_constructor(plugin_instance *p)
{
    launchbar_priv *lb; 
    int i;
    xconf *pxc;
    GtkWidget *ali;
    static gchar *launchbar_rc = "style 'launchbar-style'\n"
        "{\n"
        "GtkWidget::focus-line-width = 0\n"
        "GtkWidget::focus-padding = 0\n"
        "GtkButton::default-border = { 0, 0, 0, 0 }\n"
        "GtkButton::default-outside-border = { 0, 0, 0, 0 }\n"
        "}\n"
        "widget '*' style 'launchbar-style'";
   
    ENTER;
    lb = (launchbar_priv *) p;
    lb->iconsize = p->panel->max_elem_height;
    DBG("iconsize=%d\n", lb->iconsize);

    gtk_widget_set_name(p->pwid, "launchbar");
    gtk_rc_parse_string(launchbar_rc);
    //get_button_spacing(&req, GTK_CONTAINER(p->pwid), "");
    ali = gtk_alignment_new(0.5, 0.5, 0, 0);
    g_signal_connect(G_OBJECT(ali), "size-allocate",
        (GCallback) launchbar_size_alloc, lb);
    gtk_container_set_border_width(GTK_CONTAINER(ali), 0);
    gtk_container_add(GTK_CONTAINER(p->pwid), ali);
    lb->box = gtk_bar_new(p->panel->orientation, 0,
        lb->iconsize, lb->iconsize);
    gtk_container_add(GTK_CONTAINER(ali), lb->box);
    gtk_container_set_border_width(GTK_CONTAINER (lb->box), 0);
    gtk_widget_show_all(ali);
    
    for (i = 0; (pxc = xconf_find(p->xc, "button", i)); i++)
        read_button(p, pxc);
    RET(1);
}

static plugin_class class = {
    .count       = 0,
    .type        = "launchbar",
    .name        = "Launchbar",
    .version     = "1.0",
    .description = "Bar with application launchers",
    .priv_size   = sizeof(launchbar_priv),

    .constructor = launchbar_constructor,
    .destructor  = launchbar_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
