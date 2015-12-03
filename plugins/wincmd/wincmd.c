#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "gtkbgbox.h"
//#define DEBUGPRN
#include "dbg.h"


typedef struct {
    plugin_instance plugin;
    GdkPixmap *pix;
    GdkBitmap *mask;
    int button1, button2;
    int action1, action2;
} wincmd_priv;

enum { WC_NONE, WC_ICONIFY, WC_SHADE };


xconf_enum wincmd_enum[] = {
    { .num = WC_NONE, .str = "none" },
    { .num = WC_ICONIFY, .str = "iconify" },
    { .num = WC_SHADE, .str = "shade" },
    { .num = 0, .str = NULL },
};

static void
toggle_shaded(wincmd_priv *wc, guint32 action)
{
    Window *win = NULL;
    int num, i;
    guint32 tmp2, dno;
    net_wm_window_type nwwt;
    
    ENTER;
    win = get_xaproperty (GDK_ROOT_WINDOW(), a_NET_CLIENT_LIST,
        XA_WINDOW, &num);
    if (!win)
	RET();
    if (!num)
        goto end;
    //tmp = get_xaproperty (GDK_ROOT_WINDOW(), a_NET_CURRENT_DESKTOP,
    // XA_CARDINAL, 0);
    //dno = *tmp;
    dno = get_net_current_desktop();
    DBG("wincmd: #desk=%d\n", dno);
    //XFree(tmp);
    for (i = 0; i < num; i++) {
        int skip;

        tmp2 = get_net_wm_desktop(win[i]);
        DBG("wincmd: win=0x%x dno=%d...", win[i], tmp2);
        if ((tmp2 != -1) && (tmp2 != dno)) {
            DBG("skip - not cur desk\n");
            continue;
        }
        get_net_wm_window_type(win[i], &nwwt);
        skip = (nwwt.dock || nwwt.desktop || nwwt.splash);
        if (skip) {
            DBG("skip - omnipresent window type\n");
            continue;
        }
        Xclimsg(win[i], a_NET_WM_STATE,
              action ? a_NET_WM_STATE_ADD : a_NET_WM_STATE_REMOVE,
              a_NET_WM_STATE_SHADED, 0, 0, 0);
        DBG("ok\n");
    }
    
 end:
    XFree(win);
    RET();
}

/* if all windows are iconified then open all, 
 * if any are open then iconify 'em */
static void
toggle_iconify(wincmd_priv *wc)
{
    Window *win, *awin;
    int num, i, j, dno, raise;
    guint32 tmp;
    net_wm_window_type nwwt;
    net_wm_state nws;

    ENTER;
    win = get_xaproperty (GDK_ROOT_WINDOW(), a_NET_CLIENT_LIST_STACKING,
            XA_WINDOW, &num);
    if (!win)
	RET();
    if (!num)
        goto end;
    awin = g_new(Window, num);
    dno = get_net_current_desktop();
    raise = 1;
    for (j = 0, i = 0; i < num; i++) {
        tmp = get_net_wm_desktop(win[i]);
        DBG("wincmd: win=0x%x dno=%d...", win[i], tmp);
        if ((tmp != -1) && (tmp != dno))
            continue;

        get_net_wm_window_type(win[i], &nwwt);
        tmp = (nwwt.dock || nwwt.desktop || nwwt.splash);
        if (tmp) 
            continue;

        get_net_wm_state(win[i], &nws);
        raise = raise && (nws.hidden || nws.shaded);; 
        awin[j++] = win[i];
    }
    while (j-- > 0) {
        if (raise) 
            XMapWindow (GDK_DISPLAY(), awin[j]);
        else
            XIconifyWindow(GDK_DISPLAY(), awin[j], DefaultScreen(GDK_DISPLAY()));
    }
    
    g_free(awin);
 end:
    XFree(win);
    RET();
}

static gint
clicked (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    wincmd_priv *wc = (wincmd_priv *) data;

    ENTER;
    if (event->type != GDK_BUTTON_PRESS)
        RET(FALSE);

    if (event->button == 1) {
        toggle_iconify(wc);
    } else if (event->button == 2) {
        wc->action2 = 1 - wc->action2;
        toggle_shaded(wc, wc->action2);
        DBG("wincmd: shade all\n");
    } else {
        DBG("wincmd: unsupported command\n");
    }
   
    RET(FALSE);
}

static void
wincmd_destructor(plugin_instance *p)
{
    wincmd_priv *wc = (wincmd_priv *)p;

    ENTER;
    if (wc->mask)
        g_object_unref(wc->mask);
    if (wc->pix)
        g_object_unref(wc->pix);
    RET();
}

static int
wincmd_constructor(plugin_instance *p)
{
    gchar *tooltip, *fname, *iname;
    wincmd_priv *wc;
    GtkWidget *button;
    int w, h;

    ENTER;
    wc = (wincmd_priv *) p;
    tooltip = fname = iname = NULL;
    wc->button1 = WC_ICONIFY;
    wc->button2 = WC_SHADE;
    XCG(p->xc, "Button1", &wc->button1, enum, wincmd_enum);
    XCG(p->xc, "Button2", &wc->button2, enum, wincmd_enum);
    XCG(p->xc, "Icon", &iname, str);
    XCG(p->xc, "Image", &fname, str);
    XCG(p->xc, "tooltip", &tooltip, str);
    fname = expand_tilda(fname);
    
    if (p->panel->orientation == GTK_ORIENTATION_HORIZONTAL) {
        w = -1;
        h = p->panel->max_elem_height;
    } else {
        w = p->panel->max_elem_height;
        h = -1;
    }
    button = fb_button_new(iname, fname, w, h, 0x202020, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(button), 0);
    g_signal_connect(G_OBJECT(button), "button_press_event",
          G_CALLBACK(clicked), (gpointer)wc);
  
    gtk_widget_show(button);
    gtk_container_add(GTK_CONTAINER(p->pwid), button);
    if (p->panel->transparent) 
        gtk_bgbox_set_background(button, BG_INHERIT,
            p->panel->tintcolor, p->panel->alpha);
    
    g_free(fname);
    if (tooltip) 
        gtk_widget_set_tooltip_markup(button, tooltip);

    RET(1);
}


static plugin_class class = {
    .count       = 0,
    .type        = "wincmd",
    .name        = "Show desktop",
    .version     = "1.0",
    .description = "Show Desktop button",
    .priv_size   = sizeof(wincmd_priv),
    

    .constructor = wincmd_constructor,
    .destructor = wincmd_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
