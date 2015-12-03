#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
//#include <X11/xpm.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <gdk/gdk.h>



#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "data/images/default.xpm"
#include "gtkbar.h"

/*
 * 2006.09.10 modified by Hong Jen Yee (PCMan) pcman.tw (AT) gmail.com
 * Following features are added:
 * 1. Add XUrgencyHint support. (Flashing task bar buttons, can be disabled)
 */

//#define DEBUGPRN
#include "dbg.h"

struct _taskbar;
typedef struct _task{
    struct _taskbar *tb;
    Window win;
    char *name, *iname;
    GtkWidget *button, *label, *eb;
    GtkWidget *image;
    GdkPixbuf *pixbuf;

    int refcount;
    XClassHint ch;
    int pos_x;
    int width;
    guint desktop;
    net_wm_state nws;
    net_wm_window_type nwwt;
    guint flash_timeout;
    unsigned int focused:1;
    unsigned int iconified:1;
    unsigned int urgency:1;
    unsigned int using_netwm_icon:1;
    unsigned int flash:1;
    unsigned int flash_state:1;
} task;



typedef struct _taskbar{
    plugin_instance plugin;
    Window *wins;
    Window topxwin;
    int win_num;
    GHashTable  *task_list;
    GtkWidget *hbox, *bar, *space, *menu;
    GdkPixbuf *gen_pixbuf;
    GtkStateType normal_state;
    GtkStateType focused_state;
    int num_tasks;
    int task_width;
    int vis_task_num;
    int req_width;
    int hbox_width;
    int spacing;
    guint cur_desk;
    task *focused;
    task *ptk;
    task *menutask;
    char **desk_names;
    int desk_namesno;
    int desk_num;
    guint dnd_activate;
    int alloc_no;

    int iconsize;
    int task_width_max;
    int task_height_max;
    int accept_skip_pager;
    int show_iconified;
    int show_mapped;
    int show_all_desks;
    int tooltips;
    int icons_only;
    int use_mouse_wheel;
    int use_urgency_hint;
    int discard_release_event;
} taskbar_priv;


static gchar *taskbar_rc = "style 'taskbar-style'\n"
"{\n"
"GtkWidget::focus-line-width = 0\n"
"GtkWidget::focus-padding = 0\n"
"GtkButton::default-border = { 0, 0, 0, 0 }\n"
"GtkButton::default-outside-border = { 0, 0, 0, 0 }\n"
"GtkButton::default_border = { 0, 0, 0, 0 }\n"
"GtkButton::default_outside_border = { 0, 0, 0, 0 }\n"
"}\n"
"widget '*.taskbar.*' style 'taskbar-style'";

static gboolean use_net_active=FALSE;

#define DRAG_ACTIVE_DELAY       1000
 


#define TASK_WIDTH_MAX   200
#define TASK_HEIGHT_MAX  28
#define TASK_PADDING     4
static void tk_display(taskbar_priv *tb, task *tk);
static void tb_propertynotify(taskbar_priv *tb, XEvent *ev);
static GdkFilterReturn tb_event_filter( XEvent *, GdkEvent *, taskbar_priv *);
static void taskbar_destructor(plugin_instance *p);

static gboolean tk_has_urgency( task* tk );

static void tk_flash_window( task *tk );
static void tk_unflash_window( task *tk );
static void tk_raise_window( task *tk, guint32 time );

#define TASK_VISIBLE(tb, tk) \
 ((tk)->desktop == (tb)->cur_desk || (tk)->desktop == -1 /* 0xFFFFFFFF */ )

static int
task_visible(taskbar_priv *tb, task *tk)
{
    ENTER;
    DBG("%lx: desktop=%d iconified=%d \n", tk->win, tk->desktop, tk->iconified);
    RET( (tb->show_all_desks || tk->desktop == -1
            || (tk->desktop == tb->cur_desk))
        && ((tk->iconified && tb->show_iconified)
            || (!tk->iconified && tb->show_mapped)) );
}

inline static int
accept_net_wm_state(net_wm_state *nws, int accept_skip_pager)
{
    ENTER;
    DBG("accept_skip_pager=%d  skip_taskbar=%d skip_pager=%d\n",
        accept_skip_pager,
        nws->skip_taskbar,
        nws->skip_pager);

    RET(!(nws->skip_taskbar || (accept_skip_pager && nws->skip_pager)));
}

inline static int
accept_net_wm_window_type(net_wm_window_type *nwwt)
{
    ENTER;
    DBG("desktop=%d dock=%d splash=%d\n",
        nwwt->desktop, nwwt->dock, nwwt->splash);

    RET(!(nwwt->desktop || nwwt->dock || nwwt->splash));
}



static void
tk_free_names(task *tk)
{
    ENTER;
    if ((!tk->name) != (!tk->iname)) {
        DBG("tk names partially allocated \ntk->name=%s\ntk->iname %s\n",
                tk->name, tk->iname);
    }
    if (tk->name && tk->iname) {
        g_free(tk->name);
        g_free(tk->iname);
        tk->name = tk->iname = NULL;
        tk->tb->alloc_no--;
    }
    RET();
}

static void
tk_get_names(task *tk)
{
    char *name;

    ENTER;
    tk_free_names(tk);
    name = get_utf8_property(tk->win,  a_NET_WM_NAME);
    DBG("a_NET_WM_NAME:%s\n", name);
    if (!name) {
        name = get_textproperty(tk->win,  XA_WM_NAME);
        DBG("XA_WM_NAME:%s\n", name);
    }
    if (name) {
        tk->name = g_strdup_printf(" %s ", name);
        tk->iname = g_strdup_printf("[%s]", name);
        g_free(name);
        tk->tb->alloc_no++;
    }
    RET();
}

static void
tk_set_names(task *tk)
{
    char *name;

    ENTER;
    name = tk->iconified ? tk->iname : tk->name;
    if (!tk->tb->icons_only)
        gtk_label_set_text(GTK_LABEL(tk->label), name);
    if (tk->tb->tooltips)
        gtk_widget_set_tooltip_text(tk->button, tk->name);
    RET();
}



static task *
find_task (taskbar_priv * tb, Window win)
{
    ENTER;
    RET(g_hash_table_lookup(tb->task_list, &win));
}


static void
del_task (taskbar_priv * tb, task *tk, int hdel)
{
    ENTER;
    DBG("deleting(%d)  %08x %s\n", hdel, tk->win, tk->name);
    if (tk->flash_timeout)
        g_source_remove(tk->flash_timeout);
    gtk_widget_destroy(tk->button);
    tb->num_tasks--;
    tk_free_names(tk);
    if (tb->focused == tk)
        tb->focused = NULL;
    if (hdel)
        g_hash_table_remove(tb->task_list, &tk->win);
    g_free(tk);
    RET();
}



static GdkColormap*
get_cmap (GdkPixmap *pixmap)
{
  GdkColormap *cmap;

  ENTER;
  cmap = gdk_drawable_get_colormap (pixmap);
  if (cmap)
    g_object_ref (G_OBJECT (cmap));

  if (cmap == NULL)
    {
      if (gdk_drawable_get_depth (pixmap) == 1)
        {
          /* try null cmap */
          cmap = NULL;
        }
      else
        {
          /* Try system cmap */
          GdkScreen *screen = gdk_drawable_get_screen (GDK_DRAWABLE (pixmap));
          cmap = gdk_screen_get_system_colormap (screen);
          g_object_ref (G_OBJECT (cmap));
        }
    }

  /* Be sure we aren't going to blow up due to visual mismatch */
  if (cmap &&
      (gdk_colormap_get_visual (cmap)->depth !=
       gdk_drawable_get_depth (pixmap)))
    cmap = NULL;

  RET(cmap);
}

static GdkPixbuf*
_wnck_gdk_pixbuf_get_from_pixmap (GdkPixbuf   *dest,
                                  Pixmap       xpixmap,
                                  int          src_x,
                                  int          src_y,
                                  int          dest_x,
                                  int          dest_y,
                                  int          width,
                                  int          height)
{
    GdkDrawable *drawable;
    GdkPixbuf *retval;
    GdkColormap *cmap;

    ENTER;
    retval = NULL;

    drawable = gdk_xid_table_lookup (xpixmap);

    if (drawable)
        g_object_ref (G_OBJECT (drawable));
    else
        drawable = gdk_pixmap_foreign_new (xpixmap);

    cmap = get_cmap (drawable);

    /* GDK is supposed to do this but doesn't in GTK 2.0.2,
     * fixed in 2.0.3
     */
    if (width < 0)
        gdk_drawable_get_size (drawable, &width, NULL);
    if (height < 0)
        gdk_drawable_get_size (drawable, NULL, &height);

    retval = gdk_pixbuf_get_from_drawable (dest,
          drawable,
          cmap,
          src_x, src_y,
          dest_x, dest_y,
          width, height);

    if (cmap)
        g_object_unref (G_OBJECT (cmap));
    g_object_unref (G_OBJECT (drawable));

    RET(retval);
}

static GdkPixbuf*
apply_mask (GdkPixbuf *pixbuf,
            GdkPixbuf *mask)
{
  int w, h;
  int i, j;
  GdkPixbuf *with_alpha;
  guchar *src;
  guchar *dest;
  int src_stride;
  int dest_stride;

  ENTER;
  w = MIN (gdk_pixbuf_get_width (mask), gdk_pixbuf_get_width (pixbuf));
  h = MIN (gdk_pixbuf_get_height (mask), gdk_pixbuf_get_height (pixbuf));

  with_alpha = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);

  dest = gdk_pixbuf_get_pixels (with_alpha);
  src = gdk_pixbuf_get_pixels (mask);

  dest_stride = gdk_pixbuf_get_rowstride (with_alpha);
  src_stride = gdk_pixbuf_get_rowstride (mask);

  i = 0;
  while (i < h)
    {
      j = 0;
      while (j < w)
        {
          guchar *s = src + i * src_stride + j * 3;
          guchar *d = dest + i * dest_stride + j * 4;

          /* s[0] == s[1] == s[2], they are 255 if the bit was set, 0
           * otherwise
           */
          if (s[0] == 0)
            d[3] = 0;   /* transparent */
          else
            d[3] = 255; /* opaque */

          ++j;
        }

      ++i;
    }

  RET(with_alpha);
}


static void
free_pixels (guchar *pixels, gpointer data)
{
    ENTER;
    g_free (pixels);
    RET();
}


static guchar *
argbdata_to_pixdata (gulong *argb_data, int len)
{
    guchar *p, *ret;
    int i;

    ENTER;
    ret = p = g_new (guchar, len * 4);
    if (!ret)
        RET(NULL);
    /* One could speed this up a lot. */
    i = 0;
    while (i < len) {
        guint32 argb;
        guint32 rgba;

        argb = argb_data[i];
        rgba = (argb << 8) | (argb >> 24);

        *p = rgba >> 24;
        ++p;
        *p = (rgba >> 16) & 0xff;
        ++p;
        *p = (rgba >> 8) & 0xff;
        ++p;
        *p = rgba & 0xff;
        ++p;

        ++i;
    }
    RET(ret);
}

#if 0
GdkPixbuf *
gdk_pixbuf_scale_ratio(GdkPixbuf *p, int width, int height, GdkInterpType itype)
{
    gfloat w, h, rw, rh;

    w = gdk_pixbuf_get_width(p);
    h = gdk_pixbuf_get_height(p);
    rw = w / width;
    rh = h / height;
    if (rw > rh)
        height = h / rw;
    else
        width =  w / rh;
    return  gdk_pixbuf_scale_simple(p, width, height, itype);

}
#endif



static GdkPixbuf *
get_netwm_icon(Window tkwin, int iw, int ih)
{
    gulong *data;
    GdkPixbuf *ret = NULL;
    int n;
    guchar *p;
    GdkPixbuf *src;
    int w, h;

    ENTER;
    data = get_xaproperty(tkwin, a_NET_WM_ICON, XA_CARDINAL, &n);
    if (!data)
        RET(NULL);

    /* loop through all icons in data to find best fit */
    if (0) {
        gulong *tmp;
        int len;

        len = n/sizeof(gulong);
        tmp = data;
        while (len > 2) {
            int size = tmp[0] * tmp[1];
            DBG("sub-icon: %dx%d %d bytes\n", tmp[0], tmp[1], size * 4);
            len -= size + 2;
            tmp += size;
        }
    }

    if (0) {
        int i, j, nn;

        nn = MIN(10, n);
        p = (guchar *) data;
        for (i = 0; i < nn; i++) {
            for (j = 0; j < sizeof(gulong); j++)
                ERR("%02x ", (guint) p[i*sizeof(gulong) + j]);
            ERR("\n");
        }
    }

    /* check that data indeed represents icon in w + h + ARGB[] format
     * with 16x16 dimension at least */
    if (n < (16 * 16 + 1 + 1)) {
        ERR("win %lx: icon is too small or broken (size=%d)\n", tkwin, n);
        goto out;
    }
    w = data[0];
    h = data[1];
    /* check that sizes are in 64-256 range */
    if (w < 16 || w > 256 || h < 16 || h > 256) {
        ERR("win %lx: icon size (%d, %d) is not in 64-256 range\n",
            tkwin, w, h);
        goto out;
    }

    DBG("orig  %dx%d dest %dx%d\n", w, h, iw, ih);
    p = argbdata_to_pixdata(data + 2, w * h);
    if (!p)
        goto out;
    src = gdk_pixbuf_new_from_data (p, GDK_COLORSPACE_RGB, TRUE,
        8, w, h, w * 4, free_pixels, NULL);
    if (src == NULL)
        goto out;
    ret = src;
    if (w != iw || h != ih) {
        ret = gdk_pixbuf_scale_simple(src, iw, ih, GDK_INTERP_HYPER);
        g_object_unref(src);
    }

out:
    XFree(data);
    RET(ret);
}

static GdkPixbuf *
get_wm_icon(Window tkwin, int iw, int ih)
{
    XWMHints *hints;
    Pixmap xpixmap = None, xmask = None;
    Window win;
    unsigned int w, h;
    int sd;
    GdkPixbuf *ret, *masked, *pixmap, *mask = NULL;

    ENTER;
    hints = XGetWMHints(GDK_DISPLAY(), tkwin);
    DBG("\nwm_hints %s\n", hints ? "ok" : "failed");
    if (!hints)
        RET(NULL);

    if ((hints->flags & IconPixmapHint))
        xpixmap = hints->icon_pixmap;
    if ((hints->flags & IconMaskHint))
        xmask = hints->icon_mask;
    DBG("flag=%ld xpixmap=%lx flag=%ld xmask=%lx\n",
        (hints->flags & IconPixmapHint), xpixmap,
        (hints->flags & IconMaskHint),  xmask);
    XFree(hints);
    if (xpixmap == None)
        RET(NULL);

    if (!XGetGeometry (GDK_DISPLAY(), xpixmap, &win, &sd, &sd, &w, &h,
              (guint *)&sd, (guint *)&sd)) {
        DBG("XGetGeometry failed for %x pixmap\n", (unsigned int)xpixmap);
        RET(NULL);
    }
    DBG("tkwin=%x icon pixmap w=%d h=%d\n", tkwin, w, h);
    pixmap = _wnck_gdk_pixbuf_get_from_pixmap (NULL, xpixmap, 0, 0, 0, 0, w, h);
    if (!pixmap)
        RET(NULL);
    if (xmask != None && XGetGeometry (GDK_DISPLAY(), xmask,
              &win, &sd, &sd, &w, &h, (guint *)&sd, (guint *)&sd)) {
        mask = _wnck_gdk_pixbuf_get_from_pixmap (NULL, xmask, 0, 0, 0, 0, w, h);

        if (mask) {
            masked = apply_mask (pixmap, mask);
            g_object_unref (G_OBJECT (pixmap));
            g_object_unref (G_OBJECT (mask));
            pixmap = masked;
        }
    }
    if (!pixmap)
        RET(NULL);
    ret = gdk_pixbuf_scale_simple (pixmap, iw, ih, GDK_INTERP_TILES);
    g_object_unref(pixmap);

    RET(ret);
}

inline static GdkPixbuf*
get_generic_icon(taskbar_priv *tb)
{
    ENTER;
    g_object_ref(tb->gen_pixbuf);
    RET(tb->gen_pixbuf);
}

static void
tk_update_icon (taskbar_priv *tb, task *tk, Atom a)
{
    GdkPixbuf *pixbuf;

    ENTER;
    DBG("%lx: ", tk->win);
    pixbuf = tk->pixbuf;
    if (a == a_NET_WM_ICON || a == None) {
        tk->pixbuf = get_netwm_icon(tk->win, tb->iconsize, tb->iconsize);
        tk->using_netwm_icon = (tk->pixbuf != NULL);
        DBGE("netwm_icon=%d ", tk->using_netwm_icon);
    }
    if (!tk->using_netwm_icon) {
        tk->pixbuf = get_wm_icon(tk->win, tb->iconsize, tb->iconsize);
        DBGE("wm_icon=%d ", (tk->pixbuf != NULL));
    }
    if (!tk->pixbuf) {
        tk->pixbuf = get_generic_icon(tb); // always exists
        DBGE("generic_icon=1");
    }
    if (pixbuf != tk->pixbuf) {
        if (pixbuf)
            g_object_unref(pixbuf);
    }
    DBGE(" %dx%d \n", gdk_pixbuf_get_width(tk->pixbuf),
        gdk_pixbuf_get_height(tk->pixbuf));
    RET();
}

static gboolean
on_flash_win( task *tk )
{
    tk->flash_state = !tk->flash_state;
    gtk_widget_set_state(tk->button,
          tk->flash_state ? GTK_STATE_SELECTED : tk->tb->normal_state);
    gtk_widget_queue_draw(tk->button);
    return TRUE;
}

static void
tk_flash_window( task *tk )
{
    gint interval;
    tk->flash = 1;
    tk->flash_state = !tk->flash_state;
    if (tk->flash_timeout)
        return;
    g_object_get( gtk_widget_get_settings(tk->button),
          "gtk-cursor-blink-time", &interval, NULL );
    tk->flash_timeout = g_timeout_add(interval, (GSourceFunc)on_flash_win, tk);
}

static void
tk_unflash_window( task *tk )
{
    tk->flash = tk->flash_state = 0;
    if (tk->flash_timeout) {
        g_source_remove(tk->flash_timeout);
        tk->flash_timeout = 0;
    }
}

static void
tk_raise_window( task *tk, guint32 time )
{
    if (tk->desktop != -1 && tk->desktop != tk->tb->cur_desk){
        Xclimsg(GDK_ROOT_WINDOW(), a_NET_CURRENT_DESKTOP, tk->desktop,
            0, 0, 0, 0);
        XSync (gdk_display, False);
    }
    if(use_net_active) {
        Xclimsg(tk->win, a_NET_ACTIVE_WINDOW, 2, time, 0, 0, 0);
    }
    else {
        XRaiseWindow (GDK_DISPLAY(), tk->win);
        XSetInputFocus (GDK_DISPLAY(), tk->win, RevertToNone, CurrentTime);
    }
    DBG("XRaiseWindow %x\n", tk->win);
}

static void
tk_callback_leave( GtkWidget *widget, task *tk)
{
    ENTER;
    gtk_widget_set_state(widget,
          (tk->focused) ? tk->tb->focused_state : tk->tb->normal_state);
    RET();
}


static void
tk_callback_enter( GtkWidget *widget, task *tk )
{
    ENTER;
    gtk_widget_set_state(widget,
          (tk->focused) ? tk->tb->focused_state : tk->tb->normal_state);
    RET();
}

static gboolean
delay_active_win(task* tk)
{
    tk_raise_window(tk, CurrentTime);
    tk->tb->dnd_activate = 0;
    return FALSE;
}

static gboolean
tk_callback_drag_motion( GtkWidget *widget,
      GdkDragContext *drag_context,
      gint x, gint y,
      guint time, task *tk)
{
    /* prevent excessive motion notification */
    if (!tk->tb->dnd_activate) {
        tk->tb->dnd_activate = g_timeout_add(DRAG_ACTIVE_DELAY,
              (GSourceFunc)delay_active_win, tk);
    }
    gdk_drag_status (drag_context,0,time);
    return TRUE;
}

static void
tk_callback_drag_leave (GtkWidget *widget,
      GdkDragContext *drag_context,
      guint time, task *tk)
{
    if (tk->tb->dnd_activate) {
        g_source_remove(tk->tb->dnd_activate);
        tk->tb->dnd_activate = 0;
    }
    return;
}

#if 0
static gboolean
tk_callback_expose(GtkWidget *widget, GdkEventExpose *event, task *tk)
{
    GtkStateType state;
    ENTER;
    state = (tk->focused) ? tk->tb->focused_state : tk->tb->normal_state;
    if (GTK_WIDGET_STATE(widget) != state) {
        gtk_widget_set_state(widget, state);
        gtk_widget_queue_draw(widget);
    } else {
        if( ! tk->flash || 0 == tk->flash_state ) {
            gtk_paint_box (widget->style, widget->window,
                  state,
                  (tk->focused) ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
                  &event->area, widget, "button",
                  widget->allocation.x, widget->allocation.y,
                  widget->allocation.width, widget->allocation.height);
        } else {
            gdk_draw_rectangle( widget->window,
                                widget->style->bg_gc[GTK_STATE_SELECTED],
                                TRUE, 0, 0,
                                widget->allocation.width,
                                widget->allocation.height );
        }
        /*
        _gtk_button_paint(GTK_BUTTON(widget), &event->area, state,
              (tk->focused) ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
              "button",  "buttondefault");
        */
        gtk_container_propagate_expose(GTK_CONTAINER(widget),
            GTK_BIN(widget)->child, event);
    }
    RET(FALSE);
}
#endif

static gint
tk_callback_scroll_event (GtkWidget *widget, GdkEventScroll *event, task *tk)
{
    ENTER;
    if (event->direction == GDK_SCROLL_UP) {
        GdkWindow *gdkwindow;

        gdkwindow = gdk_xid_table_lookup (tk->win);
        if (gdkwindow)
            gdk_window_show (gdkwindow);
        else
            XMapRaised (GDK_DISPLAY(), tk->win);
        XSetInputFocus (GDK_DISPLAY(), tk->win, RevertToNone, CurrentTime);
        DBG("XMapRaised  %x\n", tk->win);
    } else if (event->direction == GDK_SCROLL_DOWN) {
        DBG("tb->ptk = %x\n", (tk->tb->ptk) ? tk->tb->ptk->win : 0);
        XIconifyWindow (GDK_DISPLAY(), tk->win, DefaultScreen(GDK_DISPLAY()));
        DBG("XIconifyWindow %x\n", tk->win);
    }

    XSync (gdk_display, False);
    RET(TRUE);
}


static gboolean
tk_callback_button_press_event(GtkWidget *widget, GdkEventButton *event,
    task *tk)
{
    ENTER;
    if (event->type == GDK_BUTTON_PRESS && event->button == 3
          && event->state & GDK_CONTROL_MASK) {
        tk->tb->discard_release_event = 1;
        gtk_propagate_event(tk->tb->bar, (GdkEvent *)event);
        RET(TRUE);
    }
    RET(FALSE);
}


static gboolean
tk_callback_button_release_event(GtkWidget *widget, GdkEventButton *event,
    task *tk)
{
    ENTER;

    if (event->type == GDK_BUTTON_RELEASE && tk->tb->discard_release_event) {
        tk->tb->discard_release_event = 0;
        RET(TRUE);
    }
    if ((event->type != GDK_BUTTON_RELEASE) || (!GTK_BUTTON(widget)->in_button))
        RET(FALSE);
    DBG("win=%x\n", tk->win);
    if (event->button == 1) {
        if (tk->iconified)    {
            if(use_net_active) {
                Xclimsg(tk->win, a_NET_ACTIVE_WINDOW, 2, event->time, 0, 0, 0);
            } else {
                GdkWindow *gdkwindow;

                gdkwindow = gdk_xid_table_lookup (tk->win);
                if (gdkwindow)
                    gdk_window_show (gdkwindow);
                else
                    XMapRaised (GDK_DISPLAY(), tk->win);
                XSync (GDK_DISPLAY(), False);
                DBG("XMapRaised  %x\n", tk->win);
            }
        } else {
            DBG("tb->ptk = %x\n", (tk->tb->ptk) ? tk->tb->ptk->win : 0);
            if (tk->focused || tk == tk->tb->ptk) {
                //tk->iconified = 1;
                XIconifyWindow (GDK_DISPLAY(), tk->win,
                    DefaultScreen(GDK_DISPLAY()));
                DBG("XIconifyWindow %x\n", tk->win);
            } else {
                tk_raise_window( tk, event->time );
            }
        }
    } else if (event->button == 2) {
        Xclimsg(tk->win, a_NET_WM_STATE,
            2 /*a_NET_WM_STATE_TOGGLE*/,
            a_NET_WM_STATE_SHADED,
            0, 0, 0);
    } else if (event->button == 3) {
        /*
        XLowerWindow (GDK_DISPLAY(), tk->win);
        DBG("XLowerWindow %x\n", tk->win);
        */
        tk->tb->menutask = tk;
        gtk_menu_popup (GTK_MENU (tk->tb->menu), NULL, NULL,
            (GtkMenuPositionFunc)menu_pos,
            tk->tb->plugin.panel->orientation == GTK_ORIENTATION_HORIZONTAL
            ? NULL : widget,
            event->button, event->time);

    }
    gtk_button_released(GTK_BUTTON(widget));
    XSync (gdk_display, False);
    RET(TRUE);
}


static void
tk_update(gpointer key, task *tk, taskbar_priv *tb)
{
    ENTER;
    g_assert ((tb != NULL) && (tk != NULL));
    if (task_visible(tb, tk)) {
        gtk_widget_set_state (tk->button,
              (tk->focused) ? tb->focused_state : tb->normal_state);
        gtk_widget_queue_draw(tk->button);
        //_gtk_button_set_depressed(GTK_BUTTON(tk->button), tk->focused);
        gtk_widget_show(tk->button);

        if (tb->tooltips) {
            gtk_widget_set_tooltip_text(tk->button, tk->name);
        }
        RET();
    }
    gtk_widget_hide(tk->button);
    RET();
}

static void
tk_display(taskbar_priv *tb, task *tk)
{
    ENTER;
    tk_update(NULL, tk, tb);
    RET();
}

static void
tb_display(taskbar_priv *tb)
{
    ENTER;
    if (tb->wins)
        g_hash_table_foreach(tb->task_list, (GHFunc) tk_update, (gpointer) tb);
    RET();

}

static void
tk_build_gui(taskbar_priv *tb, task *tk)
{
    GtkWidget *w1;

    ENTER;
    g_assert ((tb != NULL) && (tk != NULL));

    /* NOTE
     * 1. the extended mask is sum of taskbar and pager needs
     * see bug [ 940441 ] pager loose track of windows
     *
     * Do not change event mask to gtk windows spwaned by this gtk client
     * this breaks gtk internals */
    if (!FBPANEL_WIN(tk->win))
        XSelectInput(GDK_DISPLAY(), tk->win,
                PropertyChangeMask | StructureNotifyMask);

    /* button */
    tk->button = gtk_button_new();
    gtk_button_set_alignment(GTK_BUTTON(tk->button), 0.5, 0.5);
    gtk_widget_show(tk->button);
    gtk_container_set_border_width(GTK_CONTAINER(tk->button), 0);
    gtk_widget_add_events (tk->button, GDK_BUTTON_RELEASE_MASK
            | GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(tk->button), "button_release_event",
          G_CALLBACK(tk_callback_button_release_event), (gpointer)tk);
    g_signal_connect(G_OBJECT(tk->button), "button_press_event",
           G_CALLBACK(tk_callback_button_press_event), (gpointer)tk);
    g_signal_connect_after (G_OBJECT (tk->button), "leave",
          G_CALLBACK (tk_callback_leave), (gpointer) tk);
    g_signal_connect_after (G_OBJECT (tk->button), "enter",
          G_CALLBACK (tk_callback_enter), (gpointer) tk);
    gtk_drag_dest_set( tk->button, 0, NULL, 0, 0);
    g_signal_connect (G_OBJECT (tk->button), "drag-motion",
          G_CALLBACK (tk_callback_drag_motion), (gpointer) tk);
    g_signal_connect (G_OBJECT (tk->button), "drag-leave",
          G_CALLBACK (tk_callback_drag_leave), (gpointer) tk);
    if (tb->use_mouse_wheel)
        g_signal_connect_after(G_OBJECT(tk->button), "scroll-event",
              G_CALLBACK(tk_callback_scroll_event), (gpointer)tk);

    /* pix */
    tk_update_icon(tb, tk, None);
    w1 = tk->image = gtk_image_new_from_pixbuf(tk->pixbuf);
    gtk_misc_set_alignment(GTK_MISC(tk->image), 0.5, 0.5);
    gtk_misc_set_padding(GTK_MISC(tk->image), 0, 0);

    if (!tb->icons_only) {
        w1 = gtk_hbox_new(FALSE, 1);
        gtk_container_set_border_width(GTK_CONTAINER(w1), 0);
        gtk_box_pack_start(GTK_BOX(w1), tk->image, FALSE, FALSE, 0);
        tk->label = gtk_label_new(tk->iconified ? tk->iname : tk->name);
        gtk_label_set_ellipsize(GTK_LABEL(tk->label), PANGO_ELLIPSIZE_END);
        gtk_misc_set_alignment(GTK_MISC(tk->label), 0.0, 0.5);
        gtk_misc_set_padding(GTK_MISC(tk->label), 0, 0);
        gtk_box_pack_start(GTK_BOX(w1), tk->label, TRUE, TRUE, 0);
    }

    gtk_container_add (GTK_CONTAINER (tk->button), w1);
    gtk_box_pack_start(GTK_BOX(tb->bar), tk->button, FALSE, TRUE, 0);
    GTK_WIDGET_UNSET_FLAGS (tk->button, GTK_CAN_FOCUS);
    GTK_WIDGET_UNSET_FLAGS (tk->button, GTK_CAN_DEFAULT);
    
    gtk_widget_show_all(tk->button);
    if (!task_visible(tb, tk)) {
        gtk_widget_hide(tk->button);
    }

    if (tk->urgency) {
        /* Flash button for window with urgency hint */
        tk_flash_window(tk);
    }
    RET();
}

static gboolean
task_remove_every(Window *win, task *tk)
{
    ENTER;
    del_task(tk->tb, tk, 0);
    RET(TRUE);
}

/* tell to remove element with zero refcount */
static gboolean
task_remove_stale(Window *win, task *tk, gpointer data)
{
    ENTER;
    if (tk->refcount-- == 0) {
        //DBG("tb_net_list <del>: 0x%x %s\n", tk->win, tk->name);
        del_task(tk->tb, tk, 0);
        RET(TRUE);
    }
    RET(FALSE);
}

/*****************************************************
 * handlers for NET actions                          *
 *****************************************************/


static void
tb_net_client_list(GtkWidget *widget, taskbar_priv *tb)
{
    int i;
    task *tk;

    ENTER;
    if (tb->wins)
        XFree(tb->wins);
    tb->wins = get_xaproperty (GDK_ROOT_WINDOW(),
        a_NET_CLIENT_LIST, XA_WINDOW, &tb->win_num);
    if (!tb->wins)
        RET();
    for (i = 0; i < tb->win_num; i++) {
        if ((tk = g_hash_table_lookup(tb->task_list, &tb->wins[i]))) {
            tk->refcount++;
        } else {
            net_wm_window_type nwwt;
            net_wm_state nws;

            get_net_wm_state(tb->wins[i], &nws);
            if (!accept_net_wm_state(&nws, tb->accept_skip_pager))
                continue;
            get_net_wm_window_type(tb->wins[i], &nwwt);
            if (!accept_net_wm_window_type(&nwwt))
                continue;

            tk = g_new0(task, 1);
            tk->refcount = 1;
            tb->num_tasks++;
            tk->win = tb->wins[i];
            tk->tb = tb;
            tk->iconified = nws.hidden;
            tk->desktop = get_net_wm_desktop(tk->win);
            tk->nws = nws;
            tk->nwwt = nwwt;
            if( tb->use_urgency_hint && tk_has_urgency(tk)) {
                tk->urgency = 1;
            }

            tk_build_gui(tb, tk);
            tk_get_names(tk);
            tk_set_names(tk);

            g_hash_table_insert(tb->task_list, &tk->win, tk);
            DBG("adding %08x(%p) %s\n", tk->win,
                FBPANEL_WIN(tk->win), tk->name);
        }
    }

    /* remove windows that arn't in the NET_CLIENT_LIST anymore */
    g_hash_table_foreach_remove(tb->task_list, (GHRFunc) task_remove_stale,
        NULL);
    tb_display(tb);
    RET();
}



static void
tb_net_current_desktop(GtkWidget *widget, taskbar_priv *tb)
{
    ENTER;
    tb->cur_desk = get_net_current_desktop();
    tb_display(tb);
    RET();
}


static void
tb_net_number_of_desktops(GtkWidget *widget, taskbar_priv *tb)
{
    ENTER;
    tb->desk_num = get_net_number_of_desktops();
    tb_display(tb);
    RET();
}


/* set new active window. if that happens to be us, then remeber
 * current focus to use it for iconify command */
static void
tb_net_active_window(GtkWidget *widget, taskbar_priv *tb)
{
    Window *f;
    task *ntk, *ctk;
    int drop_old, make_new;

    ENTER;
    g_assert (tb != NULL);
    drop_old = make_new = 0;
    ctk = tb->focused;
    ntk = NULL;
    f = get_xaproperty(GDK_ROOT_WINDOW(), a_NET_ACTIVE_WINDOW, XA_WINDOW, 0);
    DBG("FOCUS=%x\n", f ? *f : 0);
    if (!f) {
        drop_old = 1;
        tb->ptk = NULL;
    } else {
        if (*f == tb->topxwin) {
            if (ctk) {
                tb->ptk = ctk;
                drop_old = 1;
            }
        } else {
            tb->ptk = NULL;
            ntk = find_task(tb, *f);
            if (ntk != ctk) {
                drop_old = 1;
                make_new = 1;
            }
        }
        XFree(f);
    }
    if (ctk && drop_old) {
        ctk->focused = 0;
        tb->focused = NULL;
        tk_display(tb, ctk);
        DBG("old focus was dropped\n");
    }
    if (ntk && make_new) {
        ntk->focused = 1;
        tb->focused = ntk;
        tk_display(tb, ntk);
        DBG("new focus was set\n");
    }
    RET();
}

/* For older Xlib headers */
#ifndef XUrgencyHint
#define XUrgencyHint (1 << 8)
#endif

static gboolean
tk_has_urgency( task* tk )
{
    XWMHints* hints;

    tk->urgency = 0;
    hints = XGetWMHints(GDK_DISPLAY(), tk->win);
    if (hints) {
        if (hints->flags & XUrgencyHint) /* Got urgency hint */
            tk->urgency = 1;
        XFree( hints );
    }
    return tk->urgency;
}

static void
tb_propertynotify(taskbar_priv *tb, XEvent *ev)
{
    Atom at;
    Window win;

    ENTER;
    DBG("win=%x\n", ev->xproperty.window);
    at = ev->xproperty.atom;
    win = ev->xproperty.window;
    if (win != GDK_ROOT_WINDOW()) {
        task *tk = find_task(tb, win);

        if (!tk) RET();
        DBG("win=%x\n", ev->xproperty.window);
        if (at == a_NET_WM_DESKTOP) {
            DBG("NET_WM_DESKTOP\n");
            tk->desktop = get_net_wm_desktop(win);
            tb_display(tb);
        } else if (at == XA_WM_NAME) {
            DBG("WM_NAME\n");
            tk_get_names(tk);
            tk_set_names(tk);
        } else if (at == XA_WM_HINTS)   {
            /* some windows set their WM_HINTS icon after mapping */
            DBG("XA_WM_HINTS\n");
            tk_update_icon (tb, tk, XA_WM_HINTS);
            gtk_image_set_from_pixbuf (GTK_IMAGE(tk->image), tk->pixbuf);
            if (tb->use_urgency_hint) {
                if (tk_has_urgency(tk)) {
                    //tk->urgency = 1;
                    tk_flash_window(tk);
                } else {
                    //tk->urgency = 0;
                    tk_unflash_window(tk);
                }
            }
        } else if (at == a_NET_WM_STATE) {
            net_wm_state nws;

            DBG("_NET_WM_STATE\n");
            get_net_wm_state(tk->win, &nws);
            if (!accept_net_wm_state(&nws, tb->accept_skip_pager)) {
                del_task(tb, tk, 1);
                tb_display(tb);
            } else {
                tk->iconified = nws.hidden;
                tk_set_names(tk);
            }
        } else if (at == a_NET_WM_ICON) {
            DBG("_NET_WM_ICON\n");
            DBG("#0 %d\n", GDK_IS_PIXBUF (tk->pixbuf));
            tk_update_icon (tb, tk, a_NET_WM_ICON);
            DBG("#1 %d\n", GDK_IS_PIXBUF (tk->pixbuf));
            gtk_image_set_from_pixbuf (GTK_IMAGE(tk->image), tk->pixbuf);
            DBG("#2 %d\n", GDK_IS_PIXBUF (tk->pixbuf));
        } else if (at == a_NET_WM_WINDOW_TYPE) {
            net_wm_window_type nwwt;

            DBG("_NET_WM_WINDOW_TYPE\n");
            get_net_wm_window_type(tk->win, &nwwt);
            if (!accept_net_wm_window_type(&nwwt)) {
                del_task(tb, tk, 1);
                tb_display(tb);
            }
        } else {
            DBG("at = %d\n", at);
        }
    }
    RET();
}

static GdkFilterReturn
tb_event_filter( XEvent *xev, GdkEvent *event, taskbar_priv *tb)
{

    ENTER;
    //RET(GDK_FILTER_CONTINUE);
    g_assert(tb != NULL);
    if (xev->type == PropertyNotify )
        tb_propertynotify(tb, xev);
    RET(GDK_FILTER_CONTINUE);
}

static void
menu_close_window(GtkWidget *widget, taskbar_priv *tb)
{
    ENTER;
    DBG("win %x\n", tb->menutask->win);
    XSync (GDK_DISPLAY(), 0);
    //XKillClient(GDK_DISPLAY(), tb->menutask->win);
    Xclimsgwm(tb->menutask->win, a_WM_PROTOCOLS, a_WM_DELETE_WINDOW);
    XSync (GDK_DISPLAY(), 0);
    RET();
}


static void
menu_raise_window(GtkWidget *widget, taskbar_priv *tb)
{
    ENTER;
    DBG("win %x\n", tb->menutask->win);
    XMapRaised(GDK_DISPLAY(), tb->menutask->win);
    RET();
}


static void
menu_iconify_window(GtkWidget *widget, taskbar_priv *tb)
{
    ENTER;
    DBG("win %x\n", tb->menutask->win);
    XIconifyWindow (GDK_DISPLAY(), tb->menutask->win,
        DefaultScreen(GDK_DISPLAY()));
    RET();
}

static void
send_to_workspace(GtkWidget *widget, void *iii, taskbar_priv *tb)
{
    int dst_desktop;

    ENTER;

    dst_desktop = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "num"));
    DBG("win %x -> %d\n",  (unsigned int)tb->menutask->win, dst_desktop);
    Xclimsg(tb->menutask->win, a_NET_WM_DESKTOP, dst_desktop, 0, 0, 0, 0);

    RET();
}

#define ALL_WORKSPACES  0xFFFFFFFF

static void
tb_update_desktops_names(taskbar_priv *tb)
{
    ENTER;
    tb->desk_namesno = get_net_number_of_desktops();
    if (tb->desk_names)
        g_strfreev(tb->desk_names);
    tb->desk_names = get_utf8_property_list(GDK_ROOT_WINDOW(),
        a_NET_DESKTOP_NAMES, &(tb->desk_namesno));
    RET();
}

static void
tb_make_menu(GtkWidget *widget, taskbar_priv *tb)
{
    GtkWidget *mi, *menu, *submenu;
    gchar *buf;
    int i;

    ENTER;
    menu = gtk_menu_new ();

    mi = gtk_image_menu_item_new_with_label (_("Raise"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi),
          gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    g_signal_connect(G_OBJECT(mi), "activate",
        (GCallback)menu_raise_window, tb);
    gtk_widget_show (mi);

    mi = gtk_image_menu_item_new_with_label (_("Iconify"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi),
          gtk_image_new_from_stock(GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    g_signal_connect(G_OBJECT(mi), "activate",
        (GCallback)menu_iconify_window, tb);
    gtk_widget_show (mi);

    tb_update_desktops_names(tb);
    submenu = gtk_menu_new();
    for (i = 0; i < tb->desk_num; i++) {
        buf = g_strdup_printf("%d  %s", i + 1,
            (i < tb->desk_namesno) ? tb->desk_names[i] : "");
        mi = gtk_image_menu_item_new_with_label (buf);
        g_object_set_data(G_OBJECT(mi), "num", GINT_TO_POINTER(i));
        gtk_menu_shell_append (GTK_MENU_SHELL (submenu), mi);
        g_signal_connect(G_OBJECT(mi), "button_press_event",
            (GCallback)send_to_workspace, tb);
        g_free(buf);
    }
    mi = gtk_image_menu_item_new_with_label(_("All workspaces"));
    g_object_set_data(G_OBJECT(mi), "num", GINT_TO_POINTER(ALL_WORKSPACES));
    g_signal_connect(mi, "activate",
        (GCallback)send_to_workspace, tb);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), mi);
    gtk_widget_show_all(submenu);

    mi = gtk_image_menu_item_new_with_label(_("Move to workspace"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), submenu);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(mi),
          gtk_image_new_from_stock(GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    gtk_widget_show (mi);

    mi = gtk_separator_menu_item_new();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    gtk_widget_show (mi);

    /* we want this item to be farest from mouse pointer */
    //mi = gtk_menu_item_new_with_label ("Close Window");
    mi = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLOSE, NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    g_signal_connect(G_OBJECT(mi), "activate",
        (GCallback)menu_close_window, tb);
    gtk_widget_show (mi);

    if (tb->menu)
        gtk_widget_destroy(tb->menu);
    tb->menu = menu;
}

static void
taskbar_size_alloc(GtkWidget *widget, GtkAllocation *a,
    taskbar_priv *tb)
{
    int dim;

    ENTER;
    if (tb->plugin.panel->orientation == GTK_ORIENTATION_HORIZONTAL)
        dim = a->height / tb->task_height_max;
    else
        dim = a->width / tb->task_width_max;
    DBG("width=%d height=%d task_height_max=%d -> dim=%d\n",
        a->width, a->height, tb->task_height_max, dim);
    gtk_bar_set_dimension(GTK_BAR(tb->bar), dim);
    RET();
}

static void
taskbar_build_gui(plugin_instance *p)
{
    taskbar_priv *tb = (taskbar_priv *) p;
    GtkWidget *ali;

    ENTER;
    if (p->panel->orientation == GTK_ORIENTATION_HORIZONTAL)
        ali = gtk_alignment_new(0.0, 0.5, 0, 0);
    else
        ali = gtk_alignment_new(0.5, 0.0, 0, 0);
    g_signal_connect(G_OBJECT(ali), "size-allocate",
        (GCallback) taskbar_size_alloc, tb);
    gtk_container_set_border_width(GTK_CONTAINER(ali), 0);
    gtk_container_add(GTK_CONTAINER(p->pwid), ali);

    tb->bar = gtk_bar_new(p->panel->orientation, tb->spacing,
        tb->task_height_max, tb->task_width_max);
    gtk_container_set_border_width(GTK_CONTAINER(tb->bar), 0);
    gtk_container_add(GTK_CONTAINER(ali), tb->bar);
    gtk_widget_show_all(ali);

    tb->gen_pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)icon_xpm);

    gdk_window_add_filter(NULL, (GdkFilterFunc)tb_event_filter, tb );

    g_signal_connect (G_OBJECT (fbev), "current_desktop",
          G_CALLBACK (tb_net_current_desktop), (gpointer) tb);
    g_signal_connect (G_OBJECT (fbev), "active_window",
          G_CALLBACK (tb_net_active_window), (gpointer) tb);
    g_signal_connect (G_OBJECT (fbev), "number_of_desktops",
          G_CALLBACK (tb_net_number_of_desktops), (gpointer) tb);
    g_signal_connect (G_OBJECT (fbev), "client_list",
          G_CALLBACK (tb_net_client_list), (gpointer) tb);
    g_signal_connect (G_OBJECT (fbev), "desktop_names",
          G_CALLBACK (tb_make_menu), (gpointer) tb);
    g_signal_connect (G_OBJECT (fbev), "number_of_desktops",
          G_CALLBACK (tb_make_menu), (gpointer) tb);
    
    tb->desk_num = get_net_number_of_desktops();
    tb->cur_desk = get_net_current_desktop();
    tb->focused = NULL;
    tb->menu = NULL;
    
    tb_make_menu(NULL, tb);
    gtk_container_set_border_width(GTK_CONTAINER(p->pwid), 0);
    gtk_widget_show_all(tb->bar);
    RET();
}

void net_active_detect()
{
    int nitens;
    Atom *data;

    data = get_xaproperty(GDK_ROOT_WINDOW(), a_NET_SUPPORTED, XA_ATOM, &nitens);
    if (!data)
        return;

    while (nitens > 0)
        if(data[--nitens]==a_NET_ACTIVE_WINDOW) {
            use_net_active = TRUE;
            break;
        }

    XFree(data);
}

int
taskbar_constructor(plugin_instance *p)
{
    taskbar_priv *tb;
    GtkRequisition req;
    xconf *xc = p->xc;

    ENTER;
    tb = (taskbar_priv *) p;
    gtk_rc_parse_string(taskbar_rc);
    get_button_spacing(&req, GTK_CONTAINER(p->pwid), "");
    net_active_detect();

    tb->topxwin           = p->panel->topxwin;
    tb->tooltips          = 1;
    tb->icons_only        = 0;
    tb->accept_skip_pager = 1;
    tb->show_iconified    = 1;
    tb->show_mapped       = 1;
    tb->show_all_desks    = 0;
    tb->task_width_max    = TASK_WIDTH_MAX;
    tb->task_height_max   = p->panel->max_elem_height;
    tb->task_list         = g_hash_table_new(g_int_hash, g_int_equal);
    tb->focused_state     = GTK_STATE_ACTIVE;
    tb->normal_state      = GTK_STATE_NORMAL;
    tb->spacing           = 0;
    tb->use_mouse_wheel   = 1;
    tb->use_urgency_hint  = 1;

    XCG(xc, "tooltips", &tb->tooltips, enum, bool_enum);
    XCG(xc, "iconsonly", &tb->icons_only, enum, bool_enum);
    XCG(xc, "acceptskippager", &tb->accept_skip_pager, enum, bool_enum);
    XCG(xc, "showiconified", &tb->show_iconified, enum, bool_enum);
    XCG(xc, "showalldesks", &tb->show_all_desks, enum, bool_enum);
    XCG(xc, "showmapped", &tb->show_mapped, enum, bool_enum);
    XCG(xc, "usemousewheel", &tb->use_mouse_wheel, enum, bool_enum);
    XCG(xc, "useurgencyhint", &tb->use_urgency_hint, enum, bool_enum);
    XCG(xc, "maxtaskwidth", &tb->task_width_max, int);

    /* FIXME: until per-plugin elem height limit is ready, lets
     * use hardcoded TASK_HEIGHT_MAX pixels */
    if (tb->task_height_max > TASK_HEIGHT_MAX)
        tb->task_height_max = TASK_HEIGHT_MAX;
    if (p->panel->orientation == GTK_ORIENTATION_HORIZONTAL) {
        tb->iconsize = MIN(p->panel->ah, tb->task_height_max) - req.height;
        if (tb->icons_only)
            tb->task_width_max = tb->iconsize + req.width;
    } else {
        if (p->panel->aw <= 30)
            tb->icons_only = 1;
        tb->iconsize = MIN(p->panel->aw, tb->task_height_max) - req.height;
        if (tb->icons_only)
            tb->task_width_max = tb->iconsize + req.height;
    }
    taskbar_build_gui(p);
    tb_net_client_list(NULL, tb);
    tb_display(tb);
    tb_net_active_window(NULL, tb);
    RET(1);
}


static void
taskbar_destructor(plugin_instance *p)
{
    taskbar_priv *tb = (taskbar_priv *) p;

    ENTER;
    gdk_window_remove_filter(NULL, (GdkFilterFunc)tb_event_filter, tb);
    g_signal_handlers_disconnect_by_func(G_OBJECT (fbev),
            tb_net_current_desktop, tb);
    g_signal_handlers_disconnect_by_func(G_OBJECT (fbev),
            tb_net_active_window, tb);
    g_signal_handlers_disconnect_by_func(G_OBJECT (fbev),
            tb_net_number_of_desktops, tb);
    g_signal_handlers_disconnect_by_func(G_OBJECT (fbev),
            tb_net_client_list, tb);

    g_hash_table_foreach_remove(tb->task_list, (GHRFunc) task_remove_every,
            NULL);
    g_hash_table_destroy(tb->task_list);
    if (tb->wins)
        XFree(tb->wins);
    //gtk_widget_destroy(tb->bar); // destroy of p->pwid does it all
    gtk_widget_destroy(tb->menu);
    DBG("alloc_no=%d\n", tb->alloc_no);
    RET();
}

static plugin_class class = {
    .fname       = NULL,
    .count       = 0,
    .type        = "taskbar",
    .name        = "Taskbar",
    .version     = "1.0",
    .description = "Shows opened windows",
    .priv_size   = sizeof(taskbar_priv),

    .constructor = taskbar_constructor,
    .destructor  = taskbar_destructor,
};

static plugin_class *class_ptr = (plugin_class *) &class;
