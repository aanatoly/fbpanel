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


//#define DEBUGPRN
#include "dbg.h"

typedef struct wmpix_t {
    struct wmpix_t *next;
    gulong *data;
    int size;
    XClassHint ch;
} wmpix_t;

struct _icons;
typedef struct _task{
    struct _icons *ics;
    Window win;
    int refcount;
    XClassHint ch;    
} task;

typedef struct _icons{
    plugin_instance plugin;
    Window *wins;
    int win_num;
    GHashTable  *task_list;
    int num_tasks;
    wmpix_t *wmpix; 
    wmpix_t *dicon;
} icons_priv;

static void ics_propertynotify(icons_priv *ics, XEvent *ev);
static GdkFilterReturn ics_event_filter( XEvent *, GdkEvent *, icons_priv *);
static void icons_destructor(plugin_instance *p);

/******************************************/
/* Resource Release Code                  */
/******************************************/
static void
free_task(icons_priv *ics, task *tk, int hdel)
{
    ENTER;
    ics->num_tasks--; 
    if (hdel)
        g_hash_table_remove(ics->task_list, &tk->win);
    if (tk->ch.res_class)
        XFree(tk->ch.res_class);
    if (tk->ch.res_name)
        XFree(tk->ch.res_name);
    g_free(tk);
    RET();
}

static gboolean
task_remove_every(Window *win, task *tk)
{
    free_task(tk->ics, tk, 0);
    return TRUE;
}


static void
drop_config(icons_priv *ics)
{
    wmpix_t *wp;

    ENTER;
    /* free application icons */
    while (ics->wmpix)
    {
        wp = ics->wmpix;
        ics->wmpix = ics->wmpix->next;
        g_free(wp->ch.res_name);
        g_free(wp->ch.res_class);
        g_free(wp->data);
        g_free(wp);
    }

    /* free default icon */
    if (ics->dicon)
    {
        g_free(ics->dicon->data);
        g_free(ics->dicon);
        ics->dicon = NULL;
    }

    /* free task list */
    g_hash_table_foreach_remove(ics->task_list,
        (GHRFunc) task_remove_every, (gpointer)ics);

    if (ics->wins)
    {
        DBG("free ics->wins\n");
        XFree(ics->wins);
        ics->wins = NULL;
    }
    RET();
}

static void
get_wmclass(task *tk)
{
    ENTER;
    if (tk->ch.res_name)
        XFree(tk->ch.res_name);
    if (tk->ch.res_class)
        XFree(tk->ch.res_class);
    if (!XGetClassHint (gdk_display, tk->win, &tk->ch)) 
        tk->ch.res_class = tk->ch.res_name = NULL;
    DBG("name=%s class=%s\n", tk->ch.res_name, tk->ch.res_class);
    RET();
}




static inline task *
find_task (icons_priv * ics, Window win)
{
    ENTER;
    RET(g_hash_table_lookup(ics->task_list, &win));
}


static int task_has_icon(task *tk)
{
    XWMHints *hints;
    gulong *data;
    int n;

    ENTER;
    data = get_xaproperty(tk->win, a_NET_WM_ICON, XA_CARDINAL, &n);
    if (data)
    {
        XFree(data);
        RET(1);
    }
    
    hints = XGetWMHints(GDK_DISPLAY(), tk->win);
    if (hints)
    {
        if ((hints->flags & IconPixmapHint) || (hints->flags & IconMaskHint))
        {
            XFree (hints);
            RET(1);
        }
        XFree (hints);
    }
    RET(0);
}

static wmpix_t *
get_user_icon(icons_priv *ics, task *tk)
{
    wmpix_t *tmp;
    int mc, mn;

    ENTER;
    if (!(tk->ch.res_class || tk->ch.res_name))
        RET(NULL);
    DBG("\nch.res_class=[%s] ch.res_name=[%s]\n", tk->ch.res_class,
        tk->ch.res_name);

    for (tmp = ics->wmpix; tmp; tmp = tmp->next)
    { 
        DBG("tmp.res_class=[%s] tmp.res_name=[%s]\n", tmp->ch.res_class,
            tmp->ch.res_name);
        mc = !tmp->ch.res_class || !strcmp(tmp->ch.res_class, tk->ch.res_class);
        mn = !tmp->ch.res_name  || !strcmp(tmp->ch.res_name, tk->ch.res_name);
        DBG("mc=%d mn=%d\n", mc, mn);
        if (mc && mn)
        {
            DBG("match !!!!\n");
            RET(tmp);
        }
    }
    RET(NULL);
}



gulong *
pixbuf2argb (GdkPixbuf *pixbuf, int *size)
{
    gulong *data;
    guchar *pixels;
    gulong *p;
    gint width, height, stride;
    gint x, y;
    gint n_channels;

    ENTER;
    *size = 0;
    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    stride = gdk_pixbuf_get_rowstride (pixbuf);
    n_channels = gdk_pixbuf_get_n_channels (pixbuf);
      
    *size += 2 + width * height;
    p = data = g_malloc (*size * sizeof (gulong));
    *p++ = width;
    *p++ = height;
    
    pixels = gdk_pixbuf_get_pixels (pixbuf);
    
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            guchar r, g, b, a;
            
            r = pixels[y*stride + x*n_channels + 0];
            g = pixels[y*stride + x*n_channels + 1];
            b = pixels[y*stride + x*n_channels + 2];
            if (n_channels >= 4)
                a = pixels[y*stride + x*n_channels + 3];
            else
                a = 255;
            
            *p++ = a << 24 | r << 16 | g << 8 | b ;
        }
    }
    RET(data);
}



static void
set_icon_maybe (icons_priv *ics, task *tk)
{
    wmpix_t *pix;

    ENTER;
    g_assert ((ics != NULL) && (tk != NULL));
    g_return_if_fail(tk != NULL);


    pix = get_user_icon(ics, tk);
    if (!pix)
    {
        if (task_has_icon(tk))
            RET();
        pix = ics->dicon;
    } 
    if (!pix)
        RET();

    DBG("%s size=%d\n", pix->ch.res_name, pix->size);
    XChangeProperty (GDK_DISPLAY(), tk->win,
          a_NET_WM_ICON, XA_CARDINAL, 32, PropModeReplace, (guchar*) pix->data, pix->size);

    RET();
}



/* tell to remove element with zero refcount */
static gboolean
task_remove_stale(Window *win, task *tk)
{
    ENTER;
    if (tk->refcount-- == 0)
    {
        free_task(tk->ics, tk, 0);
        RET(TRUE);
    }
    RET(FALSE);
}

/*****************************************************
 * handlers for NET actions                          *
 *****************************************************/

static GdkFilterReturn
ics_event_filter( XEvent *xev, GdkEvent *event, icons_priv *ics)
{
    
    ENTER;
    g_assert(ics != NULL);
    if (xev->type == PropertyNotify)
	ics_propertynotify(ics, xev);
    RET(GDK_FILTER_CONTINUE);
}


static void
do_net_client_list(icons_priv *ics)
{
    int i;
    task *tk;
    
    ENTER;
    if (ics->wins)
    {
        DBG("free ics->wins\n");
        XFree(ics->wins);
        ics->wins = NULL;
    }
    ics->wins = get_xaproperty (GDK_ROOT_WINDOW(),
        a_NET_CLIENT_LIST, XA_WINDOW, &ics->win_num);
    if (!ics->wins) 
	RET();
    DBG("alloc ics->wins\n");
    for (i = 0; i < ics->win_num; i++)
    {
        if ((tk = g_hash_table_lookup(ics->task_list, &ics->wins[i])))
        {
            tk->refcount++;
        }
        else
        {
            tk = g_new0(task, 1);
            tk->refcount++;
            ics->num_tasks++;
            tk->win = ics->wins[i];
            tk->ics = ics;
            
            if (!FBPANEL_WIN(tk->win))
            {
                XSelectInput(GDK_DISPLAY(), tk->win,
                    PropertyChangeMask | StructureNotifyMask);
            }
            get_wmclass(tk);
            set_icon_maybe(ics, tk);
            g_hash_table_insert(ics->task_list, &tk->win, tk);
        }
    }
    
    /* remove windows that arn't in the NET_CLIENT_LIST anymore */
    g_hash_table_foreach_remove(ics->task_list,
        (GHRFunc) task_remove_stale, NULL);
    RET();
}

static void
ics_propertynotify(icons_priv *ics, XEvent *ev)
{
    Atom at;
    Window win;

    
    ENTER;
    win = ev->xproperty.window;
    at = ev->xproperty.atom;
    DBG("win=%lx at=%ld\n", win, at);
    if (win != GDK_ROOT_WINDOW())
    {
	task *tk = find_task(ics, win);

        if (!tk) 
            RET();
        if (at == XA_WM_CLASS)
        {
            get_wmclass(tk);
            set_icon_maybe(ics, tk);
        }
        else if (at == XA_WM_HINTS)
        {
            set_icon_maybe(ics, tk);
        }  
    }
    RET();
}


static int
read_application(icons_priv *ics, xconf *xc)
{
    GdkPixbuf *gp = NULL;

    gchar *fname, *iname, *appname, *classname;
    wmpix_t *wp = NULL;
    gulong *data;
    int size;
    
    ENTER;
    iname = fname = appname = classname = NULL;
    XCG(xc, "image", &fname, str);
    XCG(xc, "icon", &iname, str);
    XCG(xc, "appname", &appname, str);
    XCG(xc, "classname", &classname, str);
    fname = expand_tilda(fname);

    DBG("appname=%s classname=%s\n", appname, classname);
    if (!(fname || iname))
        goto error;
    gp = fb_pixbuf_new(iname, fname, 48, 48, FALSE);  
    if (gp)
    {
        if ((data = pixbuf2argb(gp, &size)))
        {
            wp = g_new0 (wmpix_t, 1);
            wp->next = ics->wmpix;
            wp->data = data;
            wp->size = size;
            wp->ch.res_name = g_strdup(appname);
            wp->ch.res_class = g_strdup(classname);
            DBG("read name=[%s] class=[%s]\n",
                wp->ch.res_name, wp->ch.res_class);
            ics->wmpix = wp;
        }
        g_object_unref(gp);
    }
    g_free(fname);
    RET(1);
  
error:
    g_free(fname);
    RET(0);
}

static int
read_dicon(icons_priv *ics, gchar *name)
{
    gchar *fname;
    GdkPixbuf *gp;
    int size;
    gulong *data;
    
    ENTER;
    fname = expand_tilda(name);
    if (!fname)
        RET(0);
    gp = gdk_pixbuf_new_from_file(fname, NULL);  
    if (gp)
    {
        if ((data = pixbuf2argb(gp, &size)))
        {
            ics->dicon = g_new0 (wmpix_t, 1);
            ics->dicon->data = data;
            ics->dicon->size = size;
        }
        g_object_unref(gp);
    }
    g_free(fname);    
    RET(1);
}


static int
ics_parse_config(icons_priv *ics)
{
    gchar *def_icon;
    plugin_instance *p = (plugin_instance *) ics;
    int i;
    xconf *pxc;
    
    ENTER;
    def_icon = NULL;
    XCG(p->xc, "defaulticon", &def_icon, str);
    if (def_icon && !read_dicon(ics, def_icon)) 
        goto error;

    for (i = 0; (pxc = xconf_find(p->xc, "application", i)); i++)
        if (!read_application(ics, pxc))
            goto error;
    RET(1);
    
error:
    RET(0);
}

static void
theme_changed(icons_priv *ics)
{
    ENTER;
    drop_config(ics);
    ics_parse_config(ics);
    do_net_client_list(ics);
    RET();
}

static int
icons_constructor(plugin_instance *p)
{
    icons_priv *ics;

    ENTER;
    ics = (icons_priv *) p;
    ics->task_list = g_hash_table_new(g_int_hash, g_int_equal);
    theme_changed(ics);
    g_signal_connect_swapped(G_OBJECT(gtk_icon_theme_get_default()),
        "changed", (GCallback) theme_changed, ics);
    g_signal_connect_swapped(G_OBJECT (fbev), "client_list",
        G_CALLBACK (do_net_client_list), (gpointer) ics);
    gdk_window_add_filter(NULL, (GdkFilterFunc)ics_event_filter, ics );

    RET(1);
}


static void
icons_destructor(plugin_instance *p)
{
    icons_priv *ics = (icons_priv *) p;
    
    ENTER;
    g_signal_handlers_disconnect_by_func(G_OBJECT (fbev), do_net_client_list,
        ics);
    g_signal_handlers_disconnect_by_func(G_OBJECT(gtk_icon_theme_get_default()),
        theme_changed, ics);
    gdk_window_remove_filter(NULL, (GdkFilterFunc)ics_event_filter, ics );
    drop_config(ics);
    g_hash_table_destroy(ics->task_list);
    RET();
}

static plugin_class class = {
    .count     = 0,
    .invisible = 1,

    .type        = "icons",
    .name        = "Icons",
    .version     = "1.0",
    .description = "Invisible plugin to change window icons",
    .priv_size   = sizeof(icons_priv),

    
    .constructor = icons_constructor,
    .destructor  = icons_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
