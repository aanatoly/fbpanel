/* pager.c -- pager module of fbpanel project
 *
 * Copyright (C) 2002-2003 Anatoly Asviyan <aanatoly@users.sf.net>
 *                         Joe MacDonald   <joe@deserted.net>
 *
 * This file is part of fbpanel.
 *
 * fbpanel is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * fbpanel is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sawfish; see the file COPYING.   If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>


#include <gdk-pixbuf/gdk-pixbuf.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "gtkbgbox.h"

//#define DEBUGPRN
#include "dbg.h"



/* managed window: all related info that wm holds about its managed windows */
typedef struct _task {
    Window win;
    int x, y;
    guint w, h;
    gint refcount;
    guint stacking;
    guint desktop;
    char *name, *iname;
    net_wm_state nws;
    net_wm_window_type nwwt;
} task;

typedef struct _desk   desk;
typedef struct _pager_priv  pager_priv;

#define MAX_DESK_NUM   20
/* map of a desktop */
struct _desk {
    GtkWidget *da;
    Pixmap xpix;
    GdkPixmap *gpix;
    GdkPixmap *pix;
    guint no, dirty, first;
    gfloat scalew, scaleh;
    pager_priv *pg;
};

struct _pager_priv {
    plugin_instance plugin;
    GtkWidget *box;
    desk *desks[MAX_DESK_NUM];
    guint desknum;
    guint curdesk;
    gint wallpaper;
    //int dw, dh;
    gfloat /*scalex, scaley, */ratio;
    Window *wins;
    int winnum, dirty;
    GHashTable* htable;
    task *focusedtask;
    FbBg *fbbg;
    gint dah, daw;
};



#define TASK_VISIBLE(tk)                            \
 (!( (tk)->nws.hidden || (tk)->nws.skip_pager ))


static void pager_rebuild_all(FbEv *ev, pager_priv *pg);
static void desk_draw_bg(pager_priv *pg, desk *d1);
//static void pager_paint_frame(pager_priv *pg, gint no, GtkStateType state);

static void pager_destructor(plugin_instance *p);

static inline void desk_set_dirty_by_win(pager_priv *p, task *t);
static inline void desk_set_dirty(desk *d);
static inline void desk_set_dirty_all(pager_priv *pg);

/*
static void desk_clear_pixmap(desk *d);
static gboolean task_remove_stale(Window *win, task *t, pager_priv *p);
static gboolean task_remove_all(Window *win, task *t, pager_priv *p);
*/

#ifdef EXTRA_DEBUG
static pager_priv *cp;

/* debug func to print ids of all managed windows on USR2 signal */
static void
sig_usr(int signum)
{
    int j;
    task *t;

    if (signum != SIGUSR2)
        return;
    ERR("dekstop num=%d cur_desktop=%d\n", cp->desknum, cp->curdesk);
    for (j = 0; j < cp->winnum; j++) {
        if (!(t = g_hash_table_lookup(cp->htable, &cp->wins[j])))
            continue;
        ERR("win=%x desktop=%u\n", (guint) t->win, t->desktop);
    }

}
#endif


/*****************************************************************
 * Task Management Routines                                      *
 *****************************************************************/


/* tell to remove element with zero refcount */
static gboolean
task_remove_stale(Window *win, task *t, pager_priv *p)
{
    if (t->refcount-- == 0) {
        desk_set_dirty_by_win(p, t);
        if (p->focusedtask == t)
            p->focusedtask = NULL;
        DBG("del %lx\n", t->win);
        g_free(t);
        return TRUE;
    }
    return FALSE;
}

/* tell to remove element with zero refcount */
static gboolean
task_remove_all(Window *win, task *t, pager_priv *p)
{
    g_free(t);
    return TRUE;
}


static void
task_get_sizepos(task *t)
{
    Window root, junkwin;
    int rx, ry;
    guint dummy;
    XWindowAttributes win_attributes;

    ENTER;
    if (!XGetWindowAttributes(GDK_DISPLAY(), t->win, &win_attributes)) {
        if (!XGetGeometry (GDK_DISPLAY(), t->win, &root, &t->x, &t->y, &t->w, &t->h,
                  &dummy, &dummy)) {
            t->x = t->y = t->w = t->h = 2;
        }

    } else {
        XTranslateCoordinates (GDK_DISPLAY(), t->win, win_attributes.root,
              -win_attributes.border_width,
              -win_attributes.border_width,
              &rx, &ry, &junkwin);
        t->x = rx;
        t->y = ry;
        t->w = win_attributes.width;
        t->h = win_attributes.height;
        DBG("win=0x%lx WxH=%dx%d\n", t->win,t->w, t->h);
    }
    RET();
}


static void
task_update_pix(task *t, desk *d)
{
    int x, y, w, h;
    GtkWidget *widget;

    ENTER;
    g_return_if_fail(d->pix != NULL);
    if (!TASK_VISIBLE(t))
        RET();

    if (t->desktop < d->pg->desknum &&
          t->desktop != d->no)
        RET();

    x = (gfloat)t->x * d->scalew;
    y = (gfloat)t->y * d->scaleh;
    w = (gfloat)t->w * d->scalew;
    //h = (gfloat)t->h * d->scaleh;
    h = (t->nws.shaded) ? 3 : (gfloat)t->h * d->scaleh;
    if (w < 3 || h < 3)
        RET();
    widget = GTK_WIDGET(d->da);
    gdk_draw_rectangle (d->pix,
          (d->pg->focusedtask == t) ?
          widget->style->bg_gc[GTK_STATE_SELECTED] :
          widget->style->bg_gc[GTK_STATE_NORMAL],
          TRUE,
          x+1, y+1, w-1, h-1);
    gdk_draw_rectangle (d->pix,
          (d->pg->focusedtask == t) ?
          widget->style->fg_gc[GTK_STATE_SELECTED] :
          widget->style->fg_gc[GTK_STATE_NORMAL],
          FALSE,
          x, y, w, h);
    RET();
}


/*****************************************************************
 * Desk Functions                                                *
 *****************************************************************/
static void
desk_clear_pixmap(desk *d)
{
    GtkWidget *widget;

    ENTER;
    DBG("d->no=%d\n", d->no);
    if (!d->pix)
        RET();
    widget = GTK_WIDGET(d->da);
    if (d->pg->wallpaper && d->xpix != None) {
        gdk_draw_drawable (d->pix,
              widget->style->dark_gc[GTK_STATE_NORMAL],
              d->gpix,
              0, 0, 0, 0,
              widget->allocation.width,
              widget->allocation.height);
    } else {
        gdk_draw_rectangle (d->pix,
              ((d->no == d->pg->curdesk) ?
                    widget->style->dark_gc[GTK_STATE_SELECTED] :
                    widget->style->dark_gc[GTK_STATE_NORMAL]),
              TRUE,
              0, 0,
              widget->allocation.width,
              widget->allocation.height);
    }
    if (d->pg->wallpaper && d->no == d->pg->curdesk)
        gdk_draw_rectangle (d->pix,
              widget->style->light_gc[GTK_STATE_SELECTED],
              FALSE,
              0, 0,
              widget->allocation.width -1,
              widget->allocation.height -1);
    RET();
}


static void
desk_draw_bg(pager_priv *pg, desk *d1)
{
    Pixmap xpix;
    GdkPixmap *gpix;
    GdkPixbuf *p1, *p2;
    gint width, height, depth;
    FbBg *bg = pg->fbbg;
    GtkWidget *widget = d1->da;

    ENTER;
    if (d1->no) {
        desk *d0 = d1->pg->desks[0];
        if (d0->gpix && d0->xpix != None
              && d0->da->allocation.width == widget->allocation.width
              && d0->da->allocation.height == widget->allocation.height) {
            gdk_draw_drawable(d1->gpix,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  d0->gpix,0, 0, 0, 0,
                  widget->allocation.width,
                  widget->allocation.height);
            d1->xpix = d0->xpix;
            DBG("copy gpix from d0 to d%d\n", d1->no);
            RET();
        }
    }
    xpix = fb_bg_get_xrootpmap(bg);
    d1->xpix = None;
    width = widget->allocation.width;
    height = widget->allocation.height;
    DBG("w %d h %d\n", width, height);
    if (width < 3 || height < 3)
        RET();

    // create new pix
    xpix = fb_bg_get_xrootpmap(bg);
    if (xpix == None)
        RET();
    depth = gdk_drawable_get_depth(widget->window);
    gpix = fb_bg_get_xroot_pix_for_area(bg, 0, 0, gdk_screen_width(), gdk_screen_height(), depth);
    if (!gpix) {
        ERR("fb_bg_get_xroot_pix_for_area failed\n");
        RET();
    }
    p1 = gdk_pixbuf_get_from_drawable(NULL, gpix, NULL, 0, 0, 0, 0,
          gdk_screen_width(), gdk_screen_height());
    if (!p1) {
        ERR("gdk_pixbuf_get_from_drawable failed\n");
        goto err_gpix;
    }
    p2 = gdk_pixbuf_scale_simple(p1, width, height,
          //GDK_INTERP_NEAREST
          //GDK_INTERP_TILES
          //GDK_INTERP_BILINEAR
          GDK_INTERP_HYPER
        );
    if (!p2) {
        ERR("gdk_pixbuf_scale_simple failed\n");
        goto err_p1;
    }
    gdk_draw_pixbuf(d1->gpix, widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
          p2, 0, 0, 0, 0, width, height,  GDK_RGB_DITHER_NONE, 0, 0);

    d1->xpix = xpix;
    g_object_unref(p2);
 err_p1:
    g_object_unref(p1);
 err_gpix:
    g_object_unref(gpix);
    RET();
}



static inline void
desk_set_dirty(desk *d)
{
    ENTER;
    d->dirty = 1;
    gtk_widget_queue_draw(d->da);
    RET();
}

static inline void
desk_set_dirty_all(pager_priv *pg)
{
    int i;
    ENTER;
    for (i = 0; i < pg->desknum; i++)
        desk_set_dirty(pg->desks[i]);
    RET();
}

static inline void
desk_set_dirty_by_win(pager_priv *p, task *t)
{
    ENTER;
    if (t->nws.skip_pager || t->nwwt.desktop /*|| t->nwwt.dock || t->nwwt.splash*/ )
        RET();
    if (t->desktop < p->desknum)
        desk_set_dirty(p->desks[t->desktop]);
    else
        desk_set_dirty_all(p);
    RET();
}

/* Redraw the screen from the backing pixmap */
static gint
desk_expose_event (GtkWidget *widget, GdkEventExpose *event, desk *d)
{
    ENTER;
    DBG("d->no=%d\n", d->no);

    if (d->dirty) {
        pager_priv *pg = d->pg;
        task *t;
        int j;

        d->dirty = 0;
        desk_clear_pixmap(d);
        for (j = 0; j < pg->winnum; j++) {
            if (!(t = g_hash_table_lookup(pg->htable, &pg->wins[j])))
                continue;
            task_update_pix(t, d);
        }
    }
    gdk_draw_drawable(widget->window,
          widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
          d->pix,
          event->area.x, event->area.y,
          event->area.x, event->area.y,
          event->area.width, event->area.height);
    RET(FALSE);
}


/* Upon realize and every resize creates a new backing pixmap of the appropriate size */
static gint
desk_configure_event (GtkWidget *widget, GdkEventConfigure *event, desk *d)
{
    int w, h;

    ENTER;
    w = widget->allocation.width;
    h = widget->allocation.height;

    DBG("d->no=%d %dx%d %dx%d\n", d->no, w, h, d->pg->daw, d->pg->dah);
    if (d->pix)
        g_object_unref(d->pix);
    if (d->gpix)
        g_object_unref(d->gpix);
    d->pix = gdk_pixmap_new(widget->window, w, h, -1);
    if (d->pg->wallpaper) {
        d->gpix = gdk_pixmap_new(widget->window, w, h, -1);
        desk_draw_bg(d->pg, d);
    }
    d->scalew = (gfloat)h / (gfloat)gdk_screen_height();
    d->scaleh = (gfloat)w / (gfloat)gdk_screen_width();
    desk_set_dirty(d);
    RET(FALSE);
}

static gint
desk_button_press_event(GtkWidget * widget, GdkEventButton * event, desk *d)
{
    ENTER;
    if (event->type == GDK_BUTTON_PRESS && event->button == 3
          && event->state & GDK_CONTROL_MASK) {
        RET(FALSE);
    }
    DBG("s=%d\n", d->no);
    Xclimsg(GDK_ROOT_WINDOW(), a_NET_CURRENT_DESKTOP, d->no, 0, 0, 0, 0);
    RET(TRUE);
}

static void
desk_new(pager_priv *pg, int i)
{
    desk *d;

    ENTER;
    g_assert(i < pg->desknum);
    d = pg->desks[i] = g_new0(desk, 1);
    d->pg = pg;
    d->pix = NULL;
    d->dirty = 0;
    d->first = 1;
    d->no = i;

    d->da = gtk_drawing_area_new();
    gtk_widget_set_size_request(d->da, pg->daw, pg->dah);
    gtk_box_pack_start(GTK_BOX(pg->box), d->da, TRUE, TRUE, 0);
    gtk_widget_add_events (d->da, GDK_EXPOSURE_MASK
          | GDK_BUTTON_PRESS_MASK
          | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect (G_OBJECT (d->da), "expose_event",
          (GCallback) desk_expose_event, (gpointer)d);
    g_signal_connect (G_OBJECT (d->da), "configure_event",
          (GCallback) desk_configure_event, (gpointer)d);
    g_signal_connect (G_OBJECT (d->da), "button_press_event",
         (GCallback) desk_button_press_event, (gpointer)d);
    gtk_widget_show_all(d->da);
    RET();
}

static void
desk_free(pager_priv *pg, int i)
{
    desk *d;

    ENTER;
    d = pg->desks[i];
    DBG("i=%d d->no=%d d->da=%p d->pix=%p\n",
          i, d->no, d->da, d->pix);
    if (d->pix)
        g_object_unref(d->pix);
    if (d->gpix)
        g_object_unref(d->gpix);
    gtk_widget_destroy(d->da);
    g_free(d);
    RET();
}


/*****************************************************************
 * Netwm/WM Interclient Communication                            *
 *****************************************************************/

static void
do_net_active_window(FbEv *ev, pager_priv *p)
{
    Window *fwin;
    task *t;

    ENTER;
    fwin = get_xaproperty(GDK_ROOT_WINDOW(), a_NET_ACTIVE_WINDOW, XA_WINDOW, 0);
    DBG("win=%lx\n", fwin ? *fwin : 0);
    if (fwin) {
        t = g_hash_table_lookup(p->htable, fwin);
        if (t != p->focusedtask) {
            if (p->focusedtask)
                desk_set_dirty_by_win(p, p->focusedtask);
            p->focusedtask = t;
            if (t)
                desk_set_dirty_by_win(p, t);
        }
        XFree(fwin);
    } else {
        if (p->focusedtask) {
            desk_set_dirty_by_win(p, p->focusedtask);
            p->focusedtask = NULL;
        }
    }
    RET();
}

static void
do_net_current_desktop(FbEv *ev, pager_priv *pg)
{
    ENTER;
    desk_set_dirty(pg->desks[pg->curdesk]);
    gtk_widget_set_state(pg->desks[pg->curdesk]->da, GTK_STATE_NORMAL);
    //pager_paint_frame(pg, pg->curdesk, GTK_STATE_NORMAL);
    pg->curdesk =  get_net_current_desktop ();
    if (pg->curdesk >= pg->desknum)
        pg->curdesk = 0;
    desk_set_dirty(pg->desks[pg->curdesk]);
    gtk_widget_set_state(pg->desks[pg->curdesk]->da, GTK_STATE_SELECTED);
    //pager_paint_frame(pg, pg->curdesk, GTK_STATE_SELECTED);
    RET();
}


static void
do_net_client_list_stacking(FbEv *ev, pager_priv *p)
{
    int i;
    task *t;

    ENTER;
    if (p->wins)
        XFree(p->wins);
    p->wins = get_xaproperty (GDK_ROOT_WINDOW(), a_NET_CLIENT_LIST_STACKING,
          XA_WINDOW, &p->winnum);
    if (!p->wins || !p->winnum)
        RET();

    /* refresh existing tasks and add new */
    for (i = 0; i < p->winnum; i++) {
        if ((t = g_hash_table_lookup(p->htable, &p->wins[i]))) {
            t->refcount++;
            if (t->stacking != i) {
                t->stacking = i;
                desk_set_dirty_by_win(p, t);
            }
        } else {
            t = g_new0(task, 1);
            t->refcount++;
            t->win = p->wins[i];
            if (!FBPANEL_WIN(t->win))
                XSelectInput (GDK_DISPLAY(), t->win, PropertyChangeMask | StructureNotifyMask);
            t->desktop = get_net_wm_desktop(t->win);
            get_net_wm_state(t->win, &t->nws);
            get_net_wm_window_type(t->win, &t->nwwt);
            task_get_sizepos(t);
            g_hash_table_insert(p->htable, &t->win, t);
            DBG("add %lx\n", t->win);
            desk_set_dirty_by_win(p, t);
        }
    }
    /* pass throu hash table and delete stale windows */
    g_hash_table_foreach_remove(p->htable, (GHRFunc) task_remove_stale, (gpointer)p);
    RET();
}


/*****************************************************************
 * Pager Functions                                               *
 *****************************************************************/
/*
static void
pager_unmapnotify(pager_priv *p, XEvent *ev)
{
    Window win = ev->xunmap.window;
    task *t;
    if (!(t = g_hash_table_lookup(p->htable, &win)))
        RET();
    DBG("pager_unmapnotify: win=0x%x\n", win);
    RET();
    t->ws = WithdrawnState;
    desk_set_dirty_by_win(p, t);
    RET();
}
*/
static void
pager_configurenotify(pager_priv *p, XEvent *ev)
{
    Window win = ev->xconfigure.window;
    task *t;

    ENTER;

    if (!(t = g_hash_table_lookup(p->htable, &win)))
        RET();
    DBG("win=0x%lx\n", win);
    task_get_sizepos(t);
    desk_set_dirty_by_win(p, t);
    RET();
}

static void
pager_propertynotify(pager_priv *p, XEvent *ev)
{
    Atom at = ev->xproperty.atom;
    Window win = ev->xproperty.window;
    task *t;


    ENTER;
    if ((win == GDK_ROOT_WINDOW()) || !(t = g_hash_table_lookup(p->htable, &win)))
        RET();

    DBG("window=0x%lx\n", t->win);
    if (at == a_NET_WM_STATE) {
        DBG("event=NET_WM_STATE\n");
        get_net_wm_state(t->win, &t->nws);
    } else if (at == a_NET_WM_DESKTOP) {
        DBG("event=NET_WM_DESKTOP\n");
        desk_set_dirty_by_win(p, t); // to clean up desks where this task was
        t->desktop = get_net_wm_desktop(t->win);
    } else {
        RET();
    }
    desk_set_dirty_by_win(p, t);
    RET();
}

static GdkFilterReturn
pager_event_filter( XEvent *xev, GdkEvent *event, pager_priv *pg)
{
    ENTER;
    if (xev->type == PropertyNotify )
        pager_propertynotify(pg, xev);
    else if (xev->type == ConfigureNotify )
        pager_configurenotify(pg, xev);
    RET(GDK_FILTER_CONTINUE);
}
#if 0
static void
pager_paint_frame(pager_priv *pg, gint no, GtkStateType state)
{
    gint x, y, w, h, border;

    ENTER;
    RET();
    //desk_set_dirty(pg->desks[no]);
    border = gtk_container_get_border_width(GTK_CONTAINER(pg->box));
    w = pg->box->allocation.width;
    h = pg->desks[0]->da->allocation.height + border;
    x = 0;
    y = h * no;
    h += border;
    DBG("%d: %d %d %d %d\n", no, x, y, w, h);
    gtk_paint_flat_box(pg->box->style, pg->box->window,
          state,
          GTK_SHADOW_NONE,
          NULL, pg->box, "box",
          x + 1, y + 1,w, h);
    RET();

}

static gint
pager_expose_event (GtkWidget *widget, GdkEventExpose *event, pager_priv *pg)
{
    ENTER;
    DBG("curdesk=%d\n",  pg->curdesk);
    pager_paint_frame(pg, pg->curdesk, GTK_STATE_SELECTED);
    RET(TRUE);
}
#endif

static void
pager_bg_changed(FbBg *bg, pager_priv *pg)
{
    int i;

    ENTER;
    for (i = 0; i < pg->desknum; i++) {
        desk *d = pg->desks[i];
        desk_draw_bg(pg, d);
        desk_set_dirty(d);
    }
    RET();
}


static void
pager_rebuild_all(FbEv *ev, pager_priv *pg)
{
    int desknum, curdesk, dif, i;

    ENTER;
    desknum = pg->desknum;
    curdesk = pg->curdesk;

    pg->desknum = get_net_number_of_desktops();
    if (pg->desknum < 1)
        pg->desknum = 1;
    else if (pg->desknum > MAX_DESK_NUM) {
        pg->desknum = MAX_DESK_NUM;
        ERR("pager: max number of supported desks is %d\n", MAX_DESK_NUM);
    }
    pg->curdesk = get_net_current_desktop();
    if (pg->curdesk >= pg->desknum)
        pg->curdesk = 0;
    DBG("desknum=%d curdesk=%d\n", desknum, curdesk);
    DBG("pg->desknum=%d pg->curdesk=%d\n", pg->desknum, pg->curdesk);
    dif = pg->desknum - desknum;

    if (dif == 0)
        RET();

    if (dif < 0) {
        /* if desktops were deleted then delete their maps also */
        for (i = pg->desknum; i < desknum; i++)
            desk_free(pg, i);
    } else {
        for (i = desknum; i < pg->desknum; i++)
            desk_new(pg, i);
    }
    g_hash_table_foreach_remove(pg->htable, (GHRFunc) task_remove_all, (gpointer)pg);
    do_net_current_desktop(NULL, pg);
    do_net_client_list_stacking(NULL, pg);

    RET();
}

#define BORDER 1
static int
pager_constructor(plugin_instance *plug)
{
    pager_priv *pg;

    ENTER;
    pg = (pager_priv *) plug;

#ifdef EXTRA_DEBUG
    cp = pg;
    signal(SIGUSR2, sig_usr);
#endif

    pg->htable = g_hash_table_new (g_int_hash, g_int_equal);
    pg->box = plug->panel->my_box_new(TRUE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (pg->box), 0);
    gtk_widget_show(pg->box);

    gtk_bgbox_set_background(plug->pwid, BG_STYLE, 0, 0);
    gtk_container_set_border_width (GTK_CONTAINER (plug->pwid), BORDER);
    gtk_container_add(GTK_CONTAINER(plug->pwid), pg->box);

    pg->ratio = (gfloat)gdk_screen_width() / (gfloat)gdk_screen_height();
    if (plug->panel->orientation == GTK_ORIENTATION_HORIZONTAL) {
        pg->dah = plug->panel->ah - 2 * BORDER;
        pg->daw = (gfloat) pg->dah * pg->ratio;
    } else {
        pg->daw = plug->panel->aw - 2 * BORDER;
        pg->dah = (gfloat) pg->daw / pg->ratio;
    }
    pg->wallpaper = 1;
    //pg->scaley = (gfloat)pg->dh / (gfloat)gdk_screen_height();
    //pg->scalex = (gfloat)pg->dw / (gfloat)gdk_screen_width();
    XCG(plug->xc, "showwallpaper", &pg->wallpaper, enum, bool_enum);
    if (pg->wallpaper) {
        pg->fbbg = fb_bg_get_for_display();
        DBG("get fbbg %p\n", pg->fbbg);
        g_signal_connect(G_OBJECT(pg->fbbg), "changed",
            G_CALLBACK(pager_bg_changed), pg);
    }
    pager_rebuild_all(fbev, pg);

    gdk_window_add_filter(NULL, (GdkFilterFunc)pager_event_filter, pg );

    g_signal_connect (G_OBJECT (fbev), "current_desktop",
          G_CALLBACK (do_net_current_desktop), (gpointer) pg);
    g_signal_connect (G_OBJECT (fbev), "active_window",
          G_CALLBACK (do_net_active_window), (gpointer) pg);
    g_signal_connect (G_OBJECT (fbev), "number_of_desktops",
          G_CALLBACK (pager_rebuild_all), (gpointer) pg);
    g_signal_connect (G_OBJECT (fbev), "client_list_stacking",
          G_CALLBACK (do_net_client_list_stacking), (gpointer) pg);
    RET(1);
}

static void
pager_destructor(plugin_instance *p)
{
    pager_priv *pg = (pager_priv *)p;

    ENTER;
    g_signal_handlers_disconnect_by_func(G_OBJECT (fbev),
            do_net_current_desktop, pg);
    g_signal_handlers_disconnect_by_func(G_OBJECT (fbev),
            do_net_active_window, pg);
    g_signal_handlers_disconnect_by_func(G_OBJECT (fbev),
            pager_rebuild_all, pg);
    g_signal_handlers_disconnect_by_func(G_OBJECT (fbev),
            do_net_client_list_stacking, pg);
    gdk_window_remove_filter(NULL, (GdkFilterFunc)pager_event_filter, pg);
    while (pg->desknum--) {
        desk_free(pg, pg->desknum);
    }
    g_hash_table_foreach_remove(pg->htable, (GHRFunc) task_remove_all,
            (gpointer)pg);
    g_hash_table_destroy(pg->htable);
    gtk_widget_destroy(pg->box);
    if (pg->wallpaper) {
        g_signal_handlers_disconnect_by_func(G_OBJECT (pg->fbbg),
              pager_bg_changed, pg);
        DBG("put fbbg %p\n", pg->fbbg);
        g_object_unref(pg->fbbg);
    }
    if (pg->wins)
        XFree(pg->wins);
    RET();
}


static plugin_class class = {
    .fname       = NULL,
    .count       = 0,
    .type        = "pager",
    .name        = "Pager",
    .version     = "1.0",
    .description = "Pager shows thumbnails of your desktops",
    .priv_size   = sizeof(pager_priv),

    .constructor = pager_constructor,
    .destructor  = pager_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
