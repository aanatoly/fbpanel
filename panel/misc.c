

#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "misc.h"
#include "gtkbgbox.h"

//#define DEBUGPRN
#include "dbg.h"

extern panel *the_panel;

GtkIconTheme *icon_theme;

/* X11 data types */
Atom a_UTF8_STRING;
Atom a_XROOTPMAP_ID;

/* old WM spec */
Atom a_WM_STATE;
Atom a_WM_CLASS;
Atom a_WM_DELETE_WINDOW;
Atom a_WM_PROTOCOLS;

/* new NET spec */
Atom a_NET_WORKAREA;
Atom a_NET_CLIENT_LIST;
Atom a_NET_CLIENT_LIST_STACKING;
Atom a_NET_NUMBER_OF_DESKTOPS;
Atom a_NET_CURRENT_DESKTOP;
Atom a_NET_DESKTOP_NAMES;
Atom a_NET_DESKTOP_GEOMETRY;
Atom a_NET_ACTIVE_WINDOW;
Atom a_NET_CLOSE_WINDOW;
Atom a_NET_SUPPORTED;
Atom a_NET_WM_DESKTOP;
Atom a_NET_WM_STATE;
Atom a_NET_WM_STATE_SKIP_TASKBAR;
Atom a_NET_WM_STATE_SKIP_PAGER;
Atom a_NET_WM_STATE_STICKY;
Atom a_NET_WM_STATE_HIDDEN;
Atom a_NET_WM_STATE_SHADED;
Atom a_NET_WM_STATE_ABOVE;
Atom a_NET_WM_STATE_BELOW;
Atom a_NET_WM_WINDOW_TYPE;
Atom a_NET_WM_WINDOW_TYPE_DESKTOP;
Atom a_NET_WM_WINDOW_TYPE_DOCK;
Atom a_NET_WM_WINDOW_TYPE_TOOLBAR;
Atom a_NET_WM_WINDOW_TYPE_MENU;
Atom a_NET_WM_WINDOW_TYPE_UTILITY;
Atom a_NET_WM_WINDOW_TYPE_SPLASH;
Atom a_NET_WM_WINDOW_TYPE_DIALOG;
Atom a_NET_WM_WINDOW_TYPE_NORMAL;
Atom a_NET_WM_DESKTOP;
Atom a_NET_WM_NAME;
Atom a_NET_WM_VISIBLE_NAME;
Atom a_NET_WM_STRUT;
Atom a_NET_WM_STRUT_PARTIAL;
Atom a_NET_WM_ICON;
Atom a_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR;

xconf_enum allign_enum[] = {
    { .num = ALLIGN_LEFT, .str = c_("left") },
    { .num = ALLIGN_RIGHT, .str = c_("right") },
    { .num = ALLIGN_CENTER, .str = c_("center")},
    { .num = 0, .str = NULL },
};
xconf_enum edge_enum[] = {
    { .num = EDGE_LEFT, .str = c_("left") },
    { .num = EDGE_RIGHT, .str = c_("right") },
    { .num = EDGE_TOP, .str = c_("top") },
    { .num = EDGE_BOTTOM, .str = c_("bottom") },
    { .num = 0, .str = NULL },
};
xconf_enum widthtype_enum[] = {
    { .num = WIDTH_REQUEST, .str = "request" , .desc = c_("dynamic") },
    { .num = WIDTH_PIXEL, .str = "pixel" , .desc = c_("pixels") },
    { .num = WIDTH_PERCENT, .str = "percent", .desc = c_("% of screen") },
    { .num = 0, .str = NULL },
};
xconf_enum heighttype_enum[] = {
    { .num = HEIGHT_PIXEL, .str = c_("pixel") },
    { .num = 0, .str = NULL },
};
xconf_enum bool_enum[] = {
    { .num = 0, .str = "false" },
    { .num = 1, .str = "true" },
    { .num = 0, .str = NULL },
};
xconf_enum pos_enum[] = {
    { .num = POS_NONE, .str = "none" },
    { .num = POS_START, .str = "start" },
    { .num = POS_END,  .str = "end" },
    { .num = 0, .str = NULL},
};
xconf_enum layer_enum[] = {
    { .num = LAYER_ABOVE, .str = c_("above") },
    { .num = LAYER_BELOW, .str = c_("below") },
    { .num = 0, .str = NULL},
};


int
str2num(xconf_enum *p, gchar *str, int defval)
{
    ENTER;
    for (;p && p->str; p++) {
        if (!g_ascii_strcasecmp(str, p->str))
            RET(p->num);
    }
    RET(defval);
}

gchar *
num2str(xconf_enum *p, int num, gchar *defval)
{
    ENTER;
    for (;p && p->str; p++) {
        if (num == p->num)
            RET(p->str);
    }
    RET(defval);
}


void resolve_atoms()
{
    ENTER;

    a_UTF8_STRING                = XInternAtom(GDK_DISPLAY(), "UTF8_STRING", False);
    a_XROOTPMAP_ID               = XInternAtom(GDK_DISPLAY(), "_XROOTPMAP_ID", False);
    a_WM_STATE                   = XInternAtom(GDK_DISPLAY(), "WM_STATE", False);
    a_WM_CLASS                   = XInternAtom(GDK_DISPLAY(), "WM_CLASS", False);
    a_WM_DELETE_WINDOW           = XInternAtom(GDK_DISPLAY(), "WM_DELETE_WINDOW", False);
    a_WM_PROTOCOLS               = XInternAtom(GDK_DISPLAY(), "WM_PROTOCOLS", False);
    a_NET_WORKAREA               = XInternAtom(GDK_DISPLAY(), "_NET_WORKAREA", False);
    a_NET_CLIENT_LIST            = XInternAtom(GDK_DISPLAY(), "_NET_CLIENT_LIST", False);
    a_NET_CLIENT_LIST_STACKING   = XInternAtom(GDK_DISPLAY(), "_NET_CLIENT_LIST_STACKING", False);
    a_NET_NUMBER_OF_DESKTOPS     = XInternAtom(GDK_DISPLAY(), "_NET_NUMBER_OF_DESKTOPS", False);
    a_NET_CURRENT_DESKTOP        = XInternAtom(GDK_DISPLAY(), "_NET_CURRENT_DESKTOP", False);
    a_NET_DESKTOP_NAMES          = XInternAtom(GDK_DISPLAY(), "_NET_DESKTOP_NAMES", False);
    a_NET_DESKTOP_GEOMETRY       = XInternAtom(GDK_DISPLAY(), "_NET_DESKTOP_GEOMETRY", False);
    a_NET_ACTIVE_WINDOW          = XInternAtom(GDK_DISPLAY(), "_NET_ACTIVE_WINDOW", False);
    a_NET_SUPPORTED              = XInternAtom(GDK_DISPLAY(), "_NET_SUPPORTED", False);
    a_NET_WM_DESKTOP             = XInternAtom(GDK_DISPLAY(), "_NET_WM_DESKTOP", False);
    a_NET_WM_STATE               = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE", False);
    a_NET_WM_STATE_SKIP_TASKBAR  = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_SKIP_TASKBAR", False);
    a_NET_WM_STATE_SKIP_PAGER    = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_SKIP_PAGER", False);
    a_NET_WM_STATE_STICKY        = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_STICKY", False);
    a_NET_WM_STATE_HIDDEN        = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_HIDDEN", False);
    a_NET_WM_STATE_SHADED        = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_SHADED", False);
    a_NET_WM_STATE_ABOVE         = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_ABOVE", False);
    a_NET_WM_STATE_BELOW         = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_BELOW", False);
    a_NET_WM_STATE_SHADED        = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_SHADED", False);
    a_NET_WM_WINDOW_TYPE         = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE", False);

    a_NET_WM_WINDOW_TYPE_DESKTOP = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    a_NET_WM_WINDOW_TYPE_DOCK    = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_DOCK", False);
    a_NET_WM_WINDOW_TYPE_TOOLBAR = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
    a_NET_WM_WINDOW_TYPE_MENU    = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_MENU", False);
    a_NET_WM_WINDOW_TYPE_UTILITY = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_UTILITY", False);
    a_NET_WM_WINDOW_TYPE_SPLASH  = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_SPLASH", False);
    a_NET_WM_WINDOW_TYPE_DIALOG  = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_DIALOG", False);
    a_NET_WM_WINDOW_TYPE_NORMAL  = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_NORMAL", False);
    a_NET_WM_DESKTOP             = XInternAtom(GDK_DISPLAY(), "_NET_WM_DESKTOP", False);
    a_NET_WM_NAME                = XInternAtom(GDK_DISPLAY(), "_NET_WM_NAME", False);
    a_NET_WM_VISIBLE_NAME        = XInternAtom(GDK_DISPLAY(), "_NET_WM_VISIBLE_NAME", False);
    a_NET_WM_STRUT               = XInternAtom(GDK_DISPLAY(), "_NET_WM_STRUT", False);
    a_NET_WM_STRUT_PARTIAL       = XInternAtom(GDK_DISPLAY(), "_NET_WM_STRUT_PARTIAL", False);
    a_NET_WM_ICON                = XInternAtom(GDK_DISPLAY(), "_NET_WM_ICON", False);
    a_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR
                                 = XInternAtom(GDK_DISPLAY(), "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);

    RET();
}


void fb_init()
{
    resolve_atoms();
    icon_theme = gtk_icon_theme_get_default();
}

void fb_free()
{
    // MUST NOT be ref'd or unref'd
    // g_object_unref(icon_theme);
}

void
Xclimsg(Window win, long type, long l0, long l1, long l2, long l3, long l4)
{
    XClientMessageEvent xev;

    xev.type = ClientMessage;
    xev.window = win;
    xev.send_event = True;
    xev.display = gdk_display;
    xev.message_type = type;
    xev.format = 32;
    xev.data.l[0] = l0;
    xev.data.l[1] = l1;
    xev.data.l[2] = l2;
    xev.data.l[3] = l3;
    xev.data.l[4] = l4;
    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
          (SubstructureNotifyMask | SubstructureRedirectMask),
          (XEvent *) & xev);
}

void
Xclimsgwm(Window win, Atom type, Atom arg)
{
    XClientMessageEvent xev;

    xev.type = ClientMessage;
    xev.window = win;
    xev.message_type = type;
    xev.format = 32;
    xev.data.l[0] = arg;
    xev.data.l[1] = GDK_CURRENT_TIME;
    XSendEvent(GDK_DISPLAY(), win, False, 0L, (XEvent *) &xev);
}


void *
get_utf8_property(Window win, Atom atom)
{

    Atom type;
    int format;
    gulong nitems;
    gulong bytes_after;
    gchar  *retval;
    int result;
    guchar *tmp = NULL;

    type = None;
    retval = NULL;
    result = XGetWindowProperty (GDK_DISPLAY(), win, atom, 0, G_MAXLONG, False,
          a_UTF8_STRING, &type, &format, &nitems,
          &bytes_after, &tmp);
    if (result != Success)
        return NULL;
    if (tmp) {
        if (type == a_UTF8_STRING && format == 8 && nitems != 0)
            retval = g_strndup ((gchar *)tmp, nitems);
        XFree (tmp);
    }
    return retval;

}

char **
get_utf8_property_list(Window win, Atom atom, int *count)
{
    Atom type;
    int format, i;
    gulong nitems;
    gulong bytes_after;
    gchar *s, **retval = NULL;
    int result;
    guchar *tmp = NULL;

    *count = 0;
    result = XGetWindowProperty(GDK_DISPLAY(), win, atom, 0, G_MAXLONG, False,
          a_UTF8_STRING, &type, &format, &nitems,
          &bytes_after, &tmp);
    if (result != Success || type != a_UTF8_STRING || tmp == NULL)
        return NULL;

    if (nitems) {
        gchar *val = (gchar *) tmp;
        DBG("res=%d(%d) nitems=%d val=%s\n", result, Success, nitems, val);
        for (i = 0; i < nitems; i++) {
            if (!val[i])
                (*count)++;
        }
        retval = g_new0 (char*, *count + 2);
        for (i = 0, s = val; i < *count; i++, s = s +  strlen (s) + 1) {
            retval[i] = g_strdup(s);
        }
        if (val[nitems-1]) {
            result = nitems - (s - val);
            DBG("val does not ends by 0, moving last %d bytes\n", result);
            g_memmove(s - 1, s, result);
            val[nitems-1] = 0;
            DBG("s=%s\n", s -1);
            retval[i] = g_strdup(s - 1);
            (*count)++;
        }
    }
    XFree (tmp);

    return retval;

}

void *
get_xaproperty (Window win, Atom prop, Atom type, int *nitems)
{
    Atom type_ret;
    int format_ret;
    unsigned long items_ret;
    unsigned long after_ret;
    unsigned char *prop_data;

    ENTER;
    prop_data = NULL;
    if (XGetWindowProperty (GDK_DISPLAY(), win, prop, 0, 0x7fffffff, False,
              type, &type_ret, &format_ret, &items_ret,
              &after_ret, &prop_data) != Success)
        RET(NULL);
    DBG("win=%x prop=%d type=%d rtype=%d rformat=%d nitems=%d\n", win, prop,
            type, type_ret, format_ret, items_ret);

    if (nitems)
        *nitems = items_ret;
    RET(prop_data);
}

static char*
text_property_to_utf8 (const XTextProperty *prop)
{
  char **list;
  int count;
  char *retval;

  ENTER;
  list = NULL;
  count = gdk_text_property_to_utf8_list (gdk_x11_xatom_to_atom (prop->encoding),
                                          prop->format,
                                          prop->value,
                                          prop->nitems,
                                          &list);

  DBG("count=%d\n", count);
  if (count == 0)
    return NULL;

  retval = list[0];
  list[0] = g_strdup (""); /* something to free */

  g_strfreev (list);

  RET(retval);
}

char *
get_textproperty(Window win, Atom atom)
{
    XTextProperty text_prop;
    char *retval;

    ENTER;
    if (XGetTextProperty(GDK_DISPLAY(), win, &text_prop, atom)) {
        DBG("format=%d enc=%d nitems=%d value=%s   \n",
              text_prop.format,
              text_prop.encoding,
              text_prop.nitems,
              text_prop.value);
        retval = text_property_to_utf8 (&text_prop);
        if (text_prop.nitems > 0)
            XFree (text_prop.value);
        RET(retval);

    }
    RET(NULL);
}


guint
get_net_number_of_desktops()
{
    guint desknum;
    guint32 *data;

    ENTER;
    data = get_xaproperty (GDK_ROOT_WINDOW(), a_NET_NUMBER_OF_DESKTOPS,
          XA_CARDINAL, 0);
    if (!data)
        RET(0);

    desknum = *data;
    XFree (data);
    RET(desknum);
}


guint
get_net_current_desktop ()
{
    guint desk;
    guint32 *data;

    ENTER;
    data = get_xaproperty (GDK_ROOT_WINDOW(), a_NET_CURRENT_DESKTOP, XA_CARDINAL, 0);
    if (!data)
        RET(0);

    desk = *data;
    XFree (data);
    RET(desk);
}

guint
get_net_wm_desktop(Window win)
{
    guint desk = 0;
    guint *data;

    ENTER;
    data = get_xaproperty (win, a_NET_WM_DESKTOP, XA_CARDINAL, 0);
    if (data) {
        desk = *data;
        XFree (data);
    } else 
        DBG("can't get desktop num for win 0x%lx", win);
    RET(desk);
}

void
get_net_wm_state(Window win, net_wm_state *nws)
{
    Atom *state;
    int num3;


    ENTER;
    bzero(nws, sizeof(*nws));
    if (!(state = get_xaproperty(win, a_NET_WM_STATE, XA_ATOM, &num3)))
        RET();

    DBG( "%x: netwm state = { ", (unsigned int)win);
    while (--num3 >= 0) {
        if (state[num3] == a_NET_WM_STATE_SKIP_PAGER) {
            DBGE("NET_WM_STATE_SKIP_PAGER ");
            nws->skip_pager = 1;
        } else if (state[num3] == a_NET_WM_STATE_SKIP_TASKBAR) {
            DBGE("NET_WM_STATE_SKIP_TASKBAR ");
            nws->skip_taskbar = 1;
        } else if (state[num3] == a_NET_WM_STATE_STICKY) {
            DBGE("NET_WM_STATE_STICKY ");
            nws->sticky = 1;
        } else if (state[num3] == a_NET_WM_STATE_HIDDEN) {
            DBGE("NET_WM_STATE_HIDDEN ");
            nws->hidden = 1;
        } else if (state[num3] == a_NET_WM_STATE_SHADED) {
            DBGE("NET_WM_STATE_SHADED ");
            nws->shaded = 1;
        } else {
            DBGE("... ");
        }
    }
    XFree(state);
    DBGE( "}\n");
    RET();
}




void
get_net_wm_window_type(Window win, net_wm_window_type *nwwt)
{
    Atom *state;
    int num3;


    ENTER;
    bzero(nwwt, sizeof(*nwwt));
    if (!(state = get_xaproperty(win, a_NET_WM_WINDOW_TYPE, XA_ATOM, &num3)))
        RET();

    DBG( "%x: netwm state = { ", (unsigned int)win);
    while (--num3 >= 0) {
        if (state[num3] == a_NET_WM_WINDOW_TYPE_DESKTOP) {
            DBG("NET_WM_WINDOW_TYPE_DESKTOP ");
            nwwt->desktop = 1;
        } else if (state[num3] == a_NET_WM_WINDOW_TYPE_DOCK) {
            DBG( "NET_WM_WINDOW_TYPE_DOCK ");
            nwwt->dock = 1;
        } else if (state[num3] == a_NET_WM_WINDOW_TYPE_TOOLBAR) {
            DBG( "NET_WM_WINDOW_TYPE_TOOLBAR ");
            nwwt->toolbar = 1;
        } else if (state[num3] == a_NET_WM_WINDOW_TYPE_MENU) {
            DBG( "NET_WM_WINDOW_TYPE_MENU ");
            nwwt->menu = 1;
        } else if (state[num3] == a_NET_WM_WINDOW_TYPE_UTILITY) {
            DBG( "NET_WM_WINDOW_TYPE_UTILITY ");
            nwwt->utility = 1;
        } else if (state[num3] == a_NET_WM_WINDOW_TYPE_SPLASH) {
            DBG( "NET_WM_WINDOW_TYPE_SPLASH ");
            nwwt->splash = 1;
        } else if (state[num3] == a_NET_WM_WINDOW_TYPE_DIALOG) {
            DBG( "NET_WM_WINDOW_TYPE_DIALOG ");
            nwwt->dialog = 1;
        } else if (state[num3] == a_NET_WM_WINDOW_TYPE_NORMAL) {
            DBG( "NET_WM_WINDOW_TYPE_NORMAL ");
            nwwt->normal = 1;
        } else {
            DBG( "... ");
        }
    }
    XFree(state);
    DBG( "}\n");
    RET();
}




static void
calculate_width(int scrw, int wtype, int allign, int margin,
      int *panw, int *x)
{
    ENTER;
    DBG("scrw=%d\n", scrw);
    DBG("IN panw=%d\n", *panw);
    //scrw -= 2;
    if (wtype == WIDTH_PERCENT) {
        /* sanity check */
        if (*panw > 100)
            *panw = 100;
        else if (*panw < 0)
            *panw = 1;
        *panw = ((gfloat) scrw * (gfloat) *panw) / 100.0;
    }
    if (*panw > scrw)
        *panw = scrw;

    if (allign != ALLIGN_CENTER) {
        if (margin > scrw) {
            ERR( "margin is bigger then edge size %d > %d. Ignoring margin\n",
                  margin, scrw);
            margin = 0;
        }
        if (wtype == WIDTH_PERCENT)
            //*panw = MAX(scrw - margin, *panw);
            ;
        else
            *panw = MIN(scrw - margin, *panw);
    }
    DBG("OUT panw=%d\n", *panw);
    if (allign == ALLIGN_LEFT)
        *x += margin;
    else if (allign == ALLIGN_RIGHT) {
        *x += scrw - *panw - margin;
        if (*x < 0)
            *x = 0;
    } else if (allign == ALLIGN_CENTER)
        *x += (scrw - *panw) / 2;
    RET();
}


void
calculate_position(panel *np)
{
    int sswidth, ssheight, minx, miny;

    ENTER;
    if (0)  {
        //if (np->curdesk < np->wa_len/4) {
        minx = np->workarea[np->curdesk*4 + 0];
        miny = np->workarea[np->curdesk*4 + 1];
        sswidth  = np->workarea[np->curdesk*4 + 2];
        ssheight = np->workarea[np->curdesk*4 + 3];
    } else {
        minx = miny = 0;
        sswidth  = gdk_screen_width();
        ssheight = gdk_screen_height();

    }

    if (np->edge == EDGE_TOP || np->edge == EDGE_BOTTOM) {
        np->aw = np->width;
        np->ax = minx;
        calculate_width(sswidth, np->widthtype, np->allign, np->margin,
              &np->aw, &np->ax);
        np->ah = np->height;
        np->ah = MIN(PANEL_HEIGHT_MAX, np->ah);
        np->ah = MAX(PANEL_HEIGHT_MIN, np->ah);
        np->ay = miny + ((np->edge == EDGE_TOP) ? 0 : (ssheight - np->ah));

    } else {
        np->ah = np->width;
        np->ay = miny;
        calculate_width(ssheight, np->widthtype, np->allign, np->margin,
              &np->ah, &np->ay);
        np->aw = np->height;
        np->aw = MIN(PANEL_HEIGHT_MAX, np->aw);
        np->aw = MAX(PANEL_HEIGHT_MIN, np->aw);
        np->ax = minx + ((np->edge == EDGE_LEFT) ? 0 : (sswidth - np->aw));
    }
    if (!np->aw)
        np->aw = 1;
    if (!np->ah)
        np->ah = 1;

    /*
    if (!np->visible) {
        DBG("pushing of screen dx=%d dy=%d\n", np->ah_dx, np->ah_dy);
        np->ax += np->ah_dx;
        np->ay += np->ah_dy;
    }
    */
    DBG("x=%d y=%d w=%d h=%d\n", np->ax, np->ay, np->aw, np->ah);
    RET();
}



gchar *
expand_tilda(gchar *file)
{
    ENTER;
    if (!file)
        RET(NULL);
    RET((file[0] == '~') ?
        g_strdup_printf("%s%s", getenv("HOME"), file+1)
        : g_strdup(file));

}


void
get_button_spacing(GtkRequisition *req, GtkContainer *parent, gchar *name)
{
    GtkWidget *b;
    //gint focus_width;
    //gint focus_pad;

    ENTER;
    b = gtk_button_new();
    gtk_widget_set_name(GTK_WIDGET(b), name);
    GTK_WIDGET_UNSET_FLAGS (b, GTK_CAN_FOCUS);
    GTK_WIDGET_UNSET_FLAGS (b, GTK_CAN_DEFAULT);
    gtk_container_set_border_width (GTK_CONTAINER (b), 0);

    if (parent)
        gtk_container_add(parent, b);

    gtk_widget_show(b);
    gtk_widget_size_request(b, req);

    gtk_widget_destroy(b);
    RET();
}


guint32 
gcolor2rgb24(GdkColor *color)
{
    guint32 i;
    guint16 r, g, b;

    ENTER;

    r = color->red * 0xFF / 0xFFFF;
    g = color->green * 0xFF / 0xFFFF;
    b = color->blue * 0xFF / 0xFFFF;
    DBG("%x %x %x ==> %x %x %x\n", color->red, color->green, color->blue, r, g, b);

    i = (color->red * 0xFF / 0xFFFF) & 0xFF;
    i <<= 8;
    i |= (color->green * 0xFF / 0xFFFF) & 0xFF;
    i <<= 8;
    i |= (color->blue * 0xFF / 0xFFFF) & 0xFF;
    DBG("i=%x\n", i);
    RET(i);
}

gchar *
gdk_color_to_RRGGBB(GdkColor *color)
{
    static gchar str[10]; // #RRGGBB + \0
    g_sprintf(str, "#%02x%02x%02x",
        color->red >> 8, color->green >> 8, color->blue >> 8);
    return str;
}

void
menu_pos(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, GtkWidget *widget)
{
    int ox, oy, w, h;

    ENTER;
    if (widget) {
        gdk_window_get_origin(widget->window, &ox, &oy);
        ox += widget->allocation.x;
        oy += widget->allocation.y;
    } else {
        gdk_display_get_pointer(gdk_display_get_default(), NULL, &ox, &oy, NULL);
        ox -= 20;
        if (ox < 0)
            ox = 0;
        oy -= 10;
        if (oy < 0)
            oy = 0;
    }
    w = GTK_WIDGET(menu)->requisition.width;
    h = GTK_WIDGET(menu)->requisition.height;
    if (the_panel->orientation == GTK_ORIENTATION_HORIZONTAL) {
        // x
        *x = ox;
        if (*x + w > gdk_screen_width())
            *x = gdk_screen_width() - w;
        // y
        if (the_panel->edge == EDGE_TOP)
            *y = the_panel->ah;
        else
            *y = gdk_screen_height() - the_panel->ah - h;
    } else {
        // x
        if (the_panel->edge == EDGE_LEFT)
            *x = the_panel->aw;
        else
            *x = gdk_screen_width() - the_panel->aw - w;
        // y
        *y = oy;
        if (*y + h > gdk_screen_height())
            *y = gdk_screen_height() - h;
    }
    DBG("w-h %d %d\n", w, h);
    *push_in = TRUE;
    RET();
}

gchar *
indent(int level)
{
    static gchar *space[] = {
        "",
        "    ",
        "        ",
        "            ",
        "                ",
    };

    if (level > sizeof(space))
        level = sizeof(space);
    RET(space[level]);
}




/**********************************************************************
 * FB Pixbuf                                                          *
 **********************************************************************/

#define MAX_SIZE 192

/* Creates a pixbuf. Several sources are tried in these order:
 *   icon named @iname
 *   file from @fname
 *   icon named "missing-image" as a fallabck, if @use_fallback is TRUE.
 * Returns pixbuf or NULL on failure
 * 
 * Result pixbuf is always smaller then MAX_SIZE 
 */
GdkPixbuf *
fb_pixbuf_new(gchar *iname, gchar *fname, int width, int height, 
        gboolean use_fallback)
{
    GdkPixbuf *pb = NULL;
    int size;

    ENTER;
    size = MIN(192, MAX(width, height));
    if (iname && !pb)
        pb = gtk_icon_theme_load_icon(icon_theme, iname, size,
            GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    if (fname && !pb)
        pb = gdk_pixbuf_new_from_file_at_size(fname, width, height, NULL);
    if (use_fallback && !pb)
        pb = gtk_icon_theme_load_icon(icon_theme, "gtk-missing-image", size,
            GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    RET(pb);
}

/* Creates hilighted version of front image to reflect mouse enter
 */ 
static GdkPixbuf *
fb_pixbuf_make_back_image(GdkPixbuf *front, gulong hicolor)
{
    GdkPixbuf *back;
    guchar *src, *up, extra[3];
    int i;

    ENTER;
    back = gdk_pixbuf_add_alpha(front, FALSE, 0, 0, 0);
    if (!back) {
        g_object_ref(G_OBJECT(front));
        RET(front);
    }
    src = gdk_pixbuf_get_pixels (back);
    for (i = 2; i >= 0; i--, hicolor >>= 8)
        extra[i] = hicolor & 0xFF;
    for (up = src + gdk_pixbuf_get_height(back) * gdk_pixbuf_get_rowstride (back);
            src < up; src+=4) {
        if (src[3] == 0)
            continue;
        for (i = 0; i < 3; i++) {
            if (src[i] + extra[i] >= 255)
                src[i] = 255;
            else
                src[i] += extra[i];
        }
    }
    RET(back);
}

#define PRESS_GAP 2
static GdkPixbuf *
fb_pixbuf_make_press_image(GdkPixbuf *front)
{
    GdkPixbuf *press, *tmp;
    int w, h;

    ENTER;
    w = gdk_pixbuf_get_width(front) - 2 * PRESS_GAP;
    h = gdk_pixbuf_get_height(front) - 2 * PRESS_GAP;
    press = gdk_pixbuf_copy(front);
    tmp = gdk_pixbuf_scale_simple(front, w, h, GDK_INTERP_HYPER);
    if (press && tmp) {
        gdk_pixbuf_fill(press, 0);
        gdk_pixbuf_copy_area(tmp, 
                0, 0,  // src_x, src_y
                w, h,  // width, height
                press, // dest_pixbuf
                PRESS_GAP, PRESS_GAP);  // dst_x, dst_y
        g_object_unref(G_OBJECT(tmp));
        RET(press);
    }
    if (press)
        g_object_unref(G_OBJECT(press));
    if (tmp)
        g_object_unref(G_OBJECT(tmp));

    g_object_ref(G_OBJECT(front));
    RET(front);
}

/**********************************************************************
 * FB Image                                                           *
 **********************************************************************/

#define PIXBBUF_NUM 3
typedef struct {
    gchar *iname, *fname;
    int width, height;
    gulong itc_id; /* icon theme change callback id */
    gulong hicolor;
    int i; /* pixbuf index */
    GdkPixbuf *pix[PIXBBUF_NUM];
} fb_image_conf_t;

static void fb_image_free(GObject *image);
static void fb_image_icon_theme_changed(GtkIconTheme *icon_theme,
        GtkWidget *image);

/* Creates an image widget from fb_pixbuf and updates it on every icon theme
 * change. To keep its internal state, image allocates some data and frees it
 * in "destroy" callback
 */
GtkWidget *
fb_image_new(gchar *iname, gchar *fname, int width, int height)
{
    GtkWidget *image;
    fb_image_conf_t *conf;

    image = gtk_image_new();
    conf = g_new0(fb_image_conf_t, 1); /* exits if fails */
    g_object_set_data(G_OBJECT(image), "conf", conf); 
    conf->itc_id = g_signal_connect_after (G_OBJECT(icon_theme),
            "changed", (GCallback) fb_image_icon_theme_changed, image);
    g_signal_connect (G_OBJECT(image),
            "destroy", (GCallback) fb_image_free, NULL);
    conf->iname = g_strdup(iname);
    conf->fname = g_strdup(fname);
    conf->width = width;
    conf->height = height;
    conf->pix[0] = fb_pixbuf_new(iname, fname, width, height, TRUE);
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), conf->pix[0]);
    gtk_widget_show(image);
    RET(image);
}

    
/* Frees image's resources
 */
static void
fb_image_free(GObject *image)
{
    fb_image_conf_t *conf;
    int i;

    ENTER;
    conf = g_object_get_data(image, "conf");
    g_signal_handler_disconnect(G_OBJECT(icon_theme), conf->itc_id);
    g_free(conf->iname);
    g_free(conf->fname);
    for (i = 0; i < PIXBBUF_NUM; i++)
        if (conf->pix[i]) 
            g_object_unref(G_OBJECT(conf->pix[i]));
    g_free(conf);
    RET();
}

/* Reloads image's pixbuf upon icon theme change
 */
static void
fb_image_icon_theme_changed(GtkIconTheme *icon_theme, GtkWidget *image)
{
    fb_image_conf_t *conf;
    int i;

    ENTER;
    conf = g_object_get_data(G_OBJECT(image), "conf");
    DBG("%s / %s\n", conf->iname, conf->fname);
    for (i = 0; i < PIXBBUF_NUM; i++)
        if (conf->pix[i]) {
            g_object_unref(G_OBJECT(conf->pix[i]));
	    conf->pix[i] = NULL;
	}
    conf->pix[0] = fb_pixbuf_new(conf->iname, conf->fname, 
            conf->width, conf->height, TRUE);
    conf->pix[1] = fb_pixbuf_make_back_image(conf->pix[0], conf->hicolor);
    conf->pix[2] = fb_pixbuf_make_press_image(conf->pix[1]);
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), conf->pix[0]);
    RET();
}


/**********************************************************************
 * FB Button                                                          *
 **********************************************************************/

static gboolean fb_button_cross(GtkImage *widget, GdkEventCrossing *event);
static gboolean fb_button_pressed(GtkWidget *widget, GdkEventButton *event);

/* Creates fb_button - bgbox with fb_image. bgbox provides pseudo transparent
 * background and event capture. fb_image follows icon theme change.
 * Additionaly, fb_button highlightes an image on mouse enter and runs simple
 * animation when clicked.
 * FIXME: @label parameter is currently ignored
 */ 
GtkWidget *
fb_button_new(gchar *iname, gchar *fname, int width, int height,
      gulong hicolor, gchar *label)
{
    GtkWidget *b, *image;
    fb_image_conf_t *conf;

    ENTER;
    b = gtk_bgbox_new();
    gtk_container_set_border_width(GTK_CONTAINER(b), 0);
    GTK_WIDGET_UNSET_FLAGS (b, GTK_CAN_FOCUS);
    image = fb_image_new(iname, fname, width, height);
    gtk_misc_set_alignment(GTK_MISC(image), 0.5, 0.5);
    gtk_misc_set_padding (GTK_MISC(image), 0, 0);
    conf = g_object_get_data(G_OBJECT(image), "conf");
    conf->hicolor = hicolor;
    conf->pix[1] = fb_pixbuf_make_back_image(conf->pix[0], conf->hicolor);
    conf->pix[2] = fb_pixbuf_make_press_image(conf->pix[1]);
    gtk_widget_add_events(b, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect_swapped (G_OBJECT (b), "enter-notify-event",
            G_CALLBACK (fb_button_cross), image);
    g_signal_connect_swapped (G_OBJECT (b), "leave-notify-event",
            G_CALLBACK (fb_button_cross), image);
    g_signal_connect_swapped (G_OBJECT (b), "button-release-event",
          G_CALLBACK (fb_button_pressed), image);
    g_signal_connect_swapped (G_OBJECT (b), "button-press-event",
          G_CALLBACK (fb_button_pressed), image);
    gtk_container_add(GTK_CONTAINER(b), image);
    gtk_widget_show_all(b);
    RET(b);
}


/* Flips front and back images upon mouse cross event - GDK_ENTER_NOTIFY
 * or GDK_LEAVE_NOTIFY
 */ 
static gboolean
fb_button_cross(GtkImage *widget, GdkEventCrossing *event)
{
    fb_image_conf_t *conf;
    int i;

    ENTER;
    conf = g_object_get_data(G_OBJECT(widget), "conf");
    if (event->type == GDK_LEAVE_NOTIFY) {
        i = 0;
    } else {
        i = 1;
    }
    if (conf->i != i) {
        conf->i = i;
        gtk_image_set_from_pixbuf(GTK_IMAGE(widget), conf->pix[i]);
    }
    DBG("%s/%s - %s - pix[%d]=%p\n", conf->iname, conf->fname, 
	(event->type == GDK_LEAVE_NOTIFY) ? "out" : "in",
	conf->i, conf->pix[conf->i]);
    RET(TRUE);
}

static gboolean
fb_button_pressed(GtkWidget *widget, GdkEventButton *event)
{
    fb_image_conf_t *conf;
    int i;

    ENTER;
    conf = g_object_get_data(G_OBJECT(widget), "conf");
    if (event->type == GDK_BUTTON_PRESS) {
        i = 2;
    } else {
        if ((event->x >=0 && event->x < widget->allocation.width)
                && (event->y >=0 && event->y < widget->allocation.height)) 
            i = 1;   
        else
            i = 0;
    }
    if (conf->i != i) {
        conf->i = i;
        gtk_image_set_from_pixbuf(GTK_IMAGE(widget), conf->pix[i]);
    }
    RET(FALSE);
}


