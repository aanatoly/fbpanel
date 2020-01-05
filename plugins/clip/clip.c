#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "bg.h"
#include "gtkbgbox.h"
#include "run.h"

//#define DEBUGPRN
#include "dbg.h"

enum wait_res_status {
    WAIT_TIMEOUT = 0xff,
    WAIT_CHECK_ANY = 0x1f,
    WAIT_NO_DATA = 0x10,
    WAIT_IS_DATA = 0x11,
    WAIT_GET_TEXT = 0x2f,
    WAIT_IS_TEXT = 0x23
};

typedef struct {
    gpointer data;
    enum wait_res_status status;

    GMainLoop *loop;
    guint timer;
} wait_res;

static void wait_for_text_loop_timeout(wait_res *res) {
    DBG("TIMEOUT 0x%x\n", res->status);
    res->status = WAIT_TIMEOUT;
    g_main_loop_quit (res->loop);
}

static void start_timeout_loop(int timeout, wait_res *res) {
    if (res->status != WAIT_CHECK_ANY && res->status != WAIT_GET_TEXT)
        return;

    res->loop = g_main_loop_new (NULL, TRUE);
    res->timer = g_timeout_add(timeout, (gboolean (*)(gpointer))wait_for_text_loop_timeout, res);

    if (g_main_loop_is_running (res->loop)) {
      GDK_THREADS_LEAVE ();
      g_main_loop_run (res->loop);
      GDK_THREADS_ENTER ();
    }
    if (res->timer) g_source_remove(res->timer);
    g_main_loop_unref (res->loop);
    res->loop = NULL;
}



typedef struct {
    gchar *text;
    char save;
} history_item;

#define NUM_CLIPS 2
typedef struct {
    plugin_instance plugin;
    GtkWidget *menu, *bg;
    GtkClipboard *clips[NUM_CLIPS];
    gint curr_clip_index[NUM_CLIPS];

    int history_size;
    int history_maxsize;
    history_item *history;

    gboolean save_full_history;
    gboolean save_history_dirty;
    gchar *historyfile;

    int save_item_count;

    int timer1;
	int timer2;
    wait_res res;
} clip_priv;


//
// clippord history mangament function
//

static void read_history_from_file(clip_priv *c) {
    FILE *f;
    int len, type;
    gchar *tmp;
    char buf[1024], *text;

    ENTER;

    DBG("Read file %s\n", c->historyfile);
    f = fopen(c->historyfile, "r");
    c->history_size = 0;
    if (f) {
        while( fgets(buf, 1024, f) ) {
            DBG("Line: %s\n", buf);

            text = index(buf, ' ');
            if (!text) {
                LOG(LOG_WARN, "wrong format in history file: %s\n", buf);
                continue;
            }
            *text = '\0';
            text = text + 1;

            len=strlen(text);
            DBG("Line lenght = %d\n", len);
            if (len > 1020) {
                ERR("Line too long!\n");
                fclose(f);
                RET();
            }

            if (len > 0) {
                text[len-1] = '\0';
                type = atoi(buf);
                if (type == 2) {
                    tmp = c->history[c->history_size-1].text;
                    c->history[c->history_size-1].text = g_strdup_printf("%s\n%s", tmp, text);
                    g_free(tmp);
                } else {
                    c->history[c->history_size].text = strdup(text);
                    c->history[c->history_size].save = type;
                    if (type)
                        c->save_item_count++;
                    c->history_size++;
                    DBG("Read from fistory file (pos=%d): save=%d text=%s\n", c->history_size,
                        c->history[c->history_size].save, c->history[c->history_size].text);
                    if (c->history_size == c->history_maxsize)
                        break;
                }
            }
        }
        fclose(f);
    }

    RET();
}
static gboolean write_history_to_file(clip_priv *c) {
    FILE *f;
    int i, save;
    char *tmp, *tmp2, *tmp3;

    ENTER;

    if (!c->save_history_dirty)
        RET(TRUE);
    c->save_history_dirty = FALSE;

    f = fopen(c->historyfile, "w");
    if (f == NULL)
        RET(FALSE);

    for (i = 0; i < c->history_size; i++) {
        if (c->save_full_history || c->history[i].save) {
            tmp = index(c->history[i].text, '\n');
            if (tmp) {
                tmp3 = tmp2 = strdup(c->history[i].text);
                tmp = tmp2 + (tmp - c->history[i].text);
                save = c->history[i].save;
                do {
                    *tmp = '\0';
                    fprintf(f, "%d %s\n", save, tmp2);
                    tmp2 = tmp + 1;
                    tmp = index(tmp2, '\n');
                    save = 2;
                } while (tmp);
                free(tmp3);
            } else {
                fprintf(f, "%d %s\n", c->history[i].save, c->history[i].text);
            }
        }
    }
    fclose(f);

    RET(TRUE);
}

static int add_to_history(const char *text, int clip_numb, clip_priv *c) {
    int i, j, find, max, min;
    history_item buf, tmp;

    //DBG("%d %d %d\n", c->history_size, c->curr_clip_index[0], c->curr_clip_index[1]);

    if ( c->history[0].text && strcmp(text, c->history[0].text) == 0 ) {
        c->curr_clip_index[clip_numb] = 0;
        return 0;
    }

    i = c->curr_clip_index[clip_numb];
    if ( i > 0 && strcmp(text, c->history[i].text) == 0 ) {
        return 0;
    }

#ifdef DEBUGPRN
    DBG("REALY ENTER - adding %s (clip_numb=%d, curr_clip_index=%d)\n",
         text, clip_numb, i);
    DBG("BEFORE:\n");
    for (i=0; i<c->history_size; i++) {
        DBG("%d -> %d %s\n", i, c->history[i].save, c->history[i].text);
    }
#endif

    c->curr_clip_index[clip_numb] = 0;
    buf = c->history[0];
    c->history[0].text = g_strdup(text);
    c->history[0].save = FALSE;

    find = 0;
    for (i = 1; i < c->history_size; i++) {
        if (strcmp(text, c->history[i].text) == 0) {
            find = i;
            break;
        }
    }

    if (find > 0) {
        DBG("find in history at pos=%d\n", find);

        c->history[0].save = c->history[find].save;
        for (i = 0; i < NUM_CLIPS; i++) {
            if (c->curr_clip_index[i] == find)
                c->curr_clip_index[i] = 0;
        }

        max = find + 1;
    } else {
        DBG("not find in history\n");
        max = c->history_size;
        if (max == c->history_maxsize) {
            DBG("calculate max moving index\n");

            // find number of protected entry by other clip source
            min = max - NUM_CLIPS;
            DBG("max=%d min=%d save_item_count=%d\n", max, min, c->save_item_count);
            for (i = 0; i < NUM_CLIPS; i++) {
                if (i == clip_numb)
                    continue;
                for (j = max-1; j > min; j--) {
                    if (c->curr_clip_index[i] == j) {
                        DBG(" is entry protected by clip = %d protect pos = %d\n", i, j);
                        max--;
                        break;
                    }
                }
            }
            DBG("max=%d\n", max);

            // fix of save_item_count
            max -= c->save_item_count;
            DBG("max=%d\n", max);
        }
    }

    for (i = 1; i < max; i++) {
        DBG("move %d (%s) -> %d\n", i-1, buf.text, i);
        tmp = c->history[i];
        c->history[i] = buf;
        buf = tmp;
    }

    if (c->history_size == 0) {
        DBG("Add first entry (%s) to history\n", text);
        c->history_size = 1;
    } else if (find == 0) {
        for (j = 0; j < NUM_CLIPS; j++) {
            if (j != clip_numb && c->curr_clip_index[j] < i && 0 <= c->curr_clip_index[j])
                c->curr_clip_index[j]++;
        }

        if (c->history_size < c->history_maxsize) {
            DBG("Add new entry %d (%s) to history\n", c->history_size, buf.text);
            c->history[c->history_size] = buf;
            c->history_size++;
        } else {
            if (max < c->history_maxsize && buf.save) {
                i = max;
                DBG2("move saved entry %d (%s) to save area -> %d\n", i-1, buf.text, i);
                tmp = c->history[i];
                c->history[i] = buf;
                buf = tmp;
            }
            DBG("Drop entry %s\n", buf.text);
            g_free(buf.text);
        }
    } else {
        g_free(tmp.text);
    }

    c->save_history_dirty = TRUE;
#ifdef DEBUGPRN
    DBG("AFTER:\n");
    for (i=0; i<c->history_size; i++) {
        DBG("%d -> %d %s\n", i, c->history[i].save, c->history[i].text);
    }
#endif
    RET(1);
}

static void get_clipboard_receiver (GtkClipboard *clipboard, gpointer data, wait_res *res) {
    if (res->status == WAIT_CHECK_ANY) {
        if ( ((GtkSelectionData*)data)->length >= 0 ) {
            res->status = WAIT_IS_DATA;
        } else {
            res->status = WAIT_NO_DATA;
        }
    } else if(res->status == WAIT_GET_TEXT) {
        res->data = (gpointer) g_strdup( (gchar*)data );
        res->status = WAIT_IS_TEXT;
    }
    if (res->loop) {
        g_main_loop_quit (res->loop);
    }
}

static gboolean get_clipboard(clip_priv *c) {
    int i;

    for (i=0; i<NUM_CLIPS; i++) {
        c->res.data = NULL;
        c->res.loop = NULL;
        c->res.status = WAIT_CHECK_ANY;
        gtk_clipboard_request_contents (c->clips[i], gdk_atom_intern_static_string ("TARGETS"),
              (GtkClipboardReceivedFunc) get_clipboard_receiver, &c->res);
        start_timeout_loop(500, &c->res);
        if (c->res.status == WAIT_NO_DATA && c->curr_clip_index[i] >= 0) {
            DBG("Restore clips %d from history\n", i);
            gtk_clipboard_set_text(c->clips[i], c->history[c->curr_clip_index[i]].text, -1);
        } else if (c->res.status == WAIT_IS_DATA) {
            //DBG("Waiting for text from clipboard %d\n", i);
            c->res.status = WAIT_GET_TEXT;
            gtk_clipboard_request_text ( c->clips[i],
                                         (GtkClipboardTextReceivedFunc)get_clipboard_receiver,
                                         &c->res );
            start_timeout_loop(500, &c->res);
            if (c->res.data) {
                add_to_history((char *)c->res.data, i, c);
                g_free(c->res.data);
            }
        } else {
            DBG("ERROR 0x%x\n", c->res.status);
        }
    }
    return TRUE;
}

static gint set_clipboard(GtkWidget *widget, GdkEventButton *event, clip_priv *c) {
    int index;

    ENTER;

    index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "num"));
    if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
        DBG("Set clip to entry %d (%s)\n", index, c->history[index].text);
        gtk_clipboard_set_text(c->clips[1], c->history[index].text, -1);
        gtk_clipboard_set_text(c->clips[0], c->history[index].text, -1);
        //c->curr_clip_index[1] = c->curr_clip_index[0] = index;
    } else if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        DBG("Switch save state for %d (%s) from %d\n", index, c->history[index].text, c->history[index].save);
        if (c->history[index].save) {
            c->history[index].save = FALSE;
            c->save_item_count--;
        } else {
            if (c->save_item_count >= c->history_maxsize - 2) {
                GtkWidget *dialog = gtk_message_dialog_new(NULL, 0, 
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "Cant set save state - no space in history list\n"\
                    "(save_item_count=%d history_maxsize=%d)",
                    c->save_item_count, c->history_maxsize);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                RET(FALSE);
            }
            c->save_item_count++;
            c->history[index].save = TRUE;
        }
        c->save_history_dirty = TRUE;
        write_history_to_file(c);
    }

    RET(FALSE);
}

//
// create and destroy clippord history menu
//

static void
menu_destroy(clip_priv *c)
{
    ENTER;
    if (c->menu) {
        gtk_widget_destroy(c->menu);
        c->menu = NULL;
    }
    RET();
}

static gboolean
menu_unmap(GtkWidget *menu, clip_priv *c)
{
    ENTER;
    if (c->plugin.panel->autohide)
        ah_start(c->plugin.panel);
    RET(FALSE);
}

static void
menu_add_item(GtkWidget *menu, clip_priv *c, int i) {
    GtkWidget *mi, *mil;
    gchar* markup_text = NULL, *text = NULL;

    ENTER;

    mi = gtk_check_menu_item_new_with_label("");
    mil = gtk_bin_get_child(GTK_BIN(mi));
    gtk_label_set_single_line_mode(GTK_LABEL(mil), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(mil), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_width_chars(GTK_LABEL(mil), 40);

    text = g_strdup(c->history[i].text);
    g_strstrip(text);
    g_strdelimit(text, "\n", ' ');
    if (i == c->curr_clip_index[0]) {
        markup_text = g_markup_printf_escaped("<b>%s</b>", text);
    } else if (i == c->curr_clip_index[1]) {
        markup_text = g_markup_printf_escaped("<b><i>%s</i></b>", text);
    } else {
        markup_text = g_markup_printf_escaped("%s", text);
    }
    gtk_label_set_markup(GTK_LABEL(mil), markup_text);
    g_free(text);
    g_free(markup_text);

    if (c->history[i].save)
        gtk_check_menu_item_set_active((GtkCheckMenuItem*)mi, TRUE);
    else
        gtk_check_menu_item_set_active((GtkCheckMenuItem*)mi, FALSE);

    g_object_set_data(G_OBJECT(mi), "num", GINT_TO_POINTER(i));
    g_signal_connect (G_OBJECT (mi), "button_press_event",
          (GCallback) set_clipboard, c);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    RET();
}

static void
menu_create(clip_priv *c)
{
    int i;

    ENTER;

    if (c->menu)
        menu_destroy(c);

    c->menu = gtk_menu_new ();
    gtk_container_set_border_width(GTK_CONTAINER(c->menu), 0);

    for (i = 0; i < c->history_size; i++) {
        menu_add_item(c->menu, c, i);
    }
    gtk_widget_show_all(c->menu);

    g_signal_connect(G_OBJECT(c->menu), "unmap",
        G_CALLBACK(menu_unmap), c);

    RET();
}

//
// create and destroy plugin
//

static gboolean
my_button_pressed(GtkWidget *widget, GdkEventButton *event, clip_priv *c)
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
        if (c->plugin.panel->autohide)
            ah_stop(c->plugin.panel);

        menu_create(c);
        gtk_menu_popup(GTK_MENU(c->menu),
            NULL, NULL, (GtkMenuPositionFunc)menu_pos, widget,
            event->button, event->time);
    }
    RET(TRUE);
}

static void
make_button(clip_priv *c)
{
    int w, h;
    gchar *fname, *iname;

    ENTER;

    /* XXX: this code is duplicated in every plugin.
     * Lets run it once in a panel */
    if (c->plugin.panel->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        w = -1;
        h = c->plugin.panel->max_elem_height;
    }
    else
    {
        w = c->plugin.panel->max_elem_height;
        h = -1;
    }
    fname = iname = NULL;
    XCG(c->plugin.xc, "image", &fname, str);
    fname = expand_tilda(fname);
    XCG(c->plugin.xc, "icon", &iname, str);
    if (fname || iname)
    {
        c->bg = fb_button_new(iname, fname, w, h, 0x282828, NULL);
        gtk_container_add(GTK_CONTAINER(c->plugin.pwid), c->bg);
        if (c->plugin.panel->transparent)
            gtk_bgbox_set_background(c->bg, BG_INHERIT, 0, 0);
        g_signal_connect (G_OBJECT (c->bg), "button-press-event",
            G_CALLBACK (my_button_pressed), c);
    }
    g_free(fname);

    RET();
}

static int
clip_constructor(plugin_instance *p)
{
    clip_priv *c;

    ENTER;

    c = (clip_priv *) p;
    c->history_maxsize = 20;
    XCG(p->xc, "HistorySize", &c->history_maxsize, int);
    DBG("history_size=%d\n", c->history_maxsize);

    c->history = calloc(c->history_maxsize, sizeof(history_item));
    c->history_size = 0;

    c->clips[0] = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    c->curr_clip_index[0] = -1;
    c->clips[1] = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    c->curr_clip_index[1] = -1;

    c->save_full_history = FALSE;
    XCG(p->xc, "SaveFullHistory", &c->save_full_history, enum, bool_enum);

    c->save_item_count = 0;
    c->historyfile = NULL;
    XCG(p->xc, "HistoryFile", &c->historyfile, str);
    c->historyfile = expand_tilda(c->historyfile);
    if (c->historyfile) {
        read_history_from_file(c);
    }

    c->timer1 = g_timeout_add(300, (gboolean (*)(gpointer))get_clipboard, (gpointer)c);
    if (c->save_full_history)
        c->timer2 = g_timeout_add(2000, (gboolean (*)(gpointer))write_history_to_file, (gpointer)c);
    else
        c->timer2 = 0;

    make_button(c);

    RET(1);
}


static void
clip_destructor(plugin_instance *p)
{
    int i;
    clip_priv *c = (clip_priv *) p;

    ENTER;

    for (i = 0; i < c->history_size; i++)
        g_free(c->history[i].text);
    free(c->history);
    menu_destroy(c);

    if (c->timer1)
        g_source_remove(c->timer1);
    if (c->timer2)
        g_source_remove(c->timer2);

    RET();
}


static plugin_class class = {
    .count       = 0,
    .type        = "clip",
    .name        = "Clip",
    .version     = "1.0",
    .description = "Clipper history plugin",
    .priv_size   = sizeof(clip_priv),

    .constructor = clip_constructor,
    .destructor  = clip_destructor,
};

static plugin_class *class_ptr = (plugin_class *) &class;
