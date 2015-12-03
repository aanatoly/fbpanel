#ifndef PANEL_H
#define PANEL_H


#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libintl.h>
#define _(String) gettext(String)
#define c_(String) String

#include "config.h"

#include "bg.h"
#include "ev.h"
#include "xconf.h"

enum { ALLIGN_CENTER, ALLIGN_LEFT, ALLIGN_RIGHT  };
enum { EDGE_BOTTOM, EDGE_LEFT, EDGE_RIGHT, EDGE_TOP };
enum { WIDTH_PERCENT, WIDTH_REQUEST, WIDTH_PIXEL };
enum { HEIGHT_PIXEL, HEIGHT_REQUEST };
enum { POS_NONE, POS_START, POS_END };
enum { HIDDEN, WAITING, VISIBLE };
enum { LAYER_ABOVE, LAYER_BELOW };

#define PANEL_HEIGHT_DEFAULT  26
#define PANEL_HEIGHT_MAX      200
#define PANEL_HEIGHT_MIN      16

#define IMGPREFIX  DATADIR "/images"

typedef struct _panel
{
    GtkWidget *topgwin;           /* main panel window */
    Window topxwin;               /* and it X window   */
    GtkWidget *lbox;              /* primary layout box */
    GtkWidget *bbox;              /* backgound box for box */
    GtkWidget *box;               /* box that contains all plugins */
    GtkWidget *menu;              /* context menu */
    GtkRequisition requisition;
    GtkWidget *(*my_box_new) (gboolean, gint);
    GtkWidget *(*my_separator_new) ();

    FbBg *bg;
    int alpha;
    guint32 tintcolor;
    GdkColor gtintcolor;
    gchar *tintcolor_name;
    
    int ax, ay, aw, ah;  /* prefferd allocation of a panel */
    int cx, cy, cw, ch;  /* current allocation (as reported by configure event) allocation */
    int allign, edge, margin;
    GtkOrientation orientation;
    int widthtype, width;
    int heighttype, height;
    int round_corners_radius;
    int max_elem_height;

    gint self_destroy;
    gint setdocktype;
    gint setstrut;
    gint round_corners;
    gint transparent;
    gint autohide;
    gint ah_far;
    gint layer;
    gint setlayer;
    
    int ah_dx, ah_dy; // autohide shift for x and y
    int height_when_hidden;
    guint hide_tout;

    int spacing;

    guint desknum;
    guint curdesk;
    guint32 *workarea;

    int plug_num;
    GList *plugins;

    gboolean (*ah_state)(struct _panel *);

    xconf *xc;
} panel;


typedef struct {
    unsigned int modal : 1;
    unsigned int sticky : 1;
    unsigned int maximized_vert : 1;
    unsigned int maximized_horz : 1;
    unsigned int shaded : 1;
    unsigned int skip_taskbar : 1;
    unsigned int skip_pager : 1;
    unsigned int hidden : 1;
    unsigned int fullscreen : 1;
    unsigned int above : 1;
    unsigned int below : 1;
} net_wm_state;

typedef struct {
    unsigned int desktop : 1;
    unsigned int dock : 1;
    unsigned int toolbar : 1;
    unsigned int menu : 1;
    unsigned int utility : 1;
    unsigned int splash : 1;
    unsigned int dialog : 1;
    unsigned int normal : 1;
} net_wm_window_type;

typedef struct {
    char *name;
    void (*cmd)(void);
} command;

extern command commands[];

extern gchar *cprofile;

extern Atom a_UTF8_STRING;
extern Atom a_XROOTPMAP_ID;

extern Atom a_WM_STATE;
extern Atom a_WM_CLASS;
extern Atom a_WM_DELETE_WINDOW;
extern Atom a_WM_PROTOCOLS;
extern Atom a_NET_WORKAREA;
extern Atom a_NET_CLIENT_LIST;
extern Atom a_NET_CLIENT_LIST_STACKING;
extern Atom a_NET_NUMBER_OF_DESKTOPS;
extern Atom a_NET_CURRENT_DESKTOP;
extern Atom a_NET_DESKTOP_NAMES;
extern Atom a_NET_DESKTOP_GEOMETRY;
extern Atom a_NET_ACTIVE_WINDOW;
extern Atom a_NET_CLOSE_WINDOW;
extern Atom a_NET_SUPPORTED;
extern Atom a_NET_WM_STATE;
extern Atom a_NET_WM_STATE_SKIP_TASKBAR;
extern Atom a_NET_WM_STATE_SKIP_PAGER;
extern Atom a_NET_WM_STATE_STICKY;
extern Atom a_NET_WM_STATE_HIDDEN;
extern Atom a_NET_WM_STATE_SHADED;
extern Atom a_NET_WM_STATE_ABOVE;
extern Atom a_NET_WM_STATE_BELOW;

#define a_NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define a_NET_WM_STATE_ADD           1    /* add/set property */
#define a_NET_WM_STATE_TOGGLE        2    /* toggle property  */

extern Atom a_NET_WM_WINDOW_TYPE;
extern Atom a_NET_WM_WINDOW_TYPE_DESKTOP;
extern Atom a_NET_WM_WINDOW_TYPE_DOCK;
extern Atom a_NET_WM_WINDOW_TYPE_TOOLBAR;
extern Atom a_NET_WM_WINDOW_TYPE_MENU;
extern Atom a_NET_WM_WINDOW_TYPE_UTILITY;
extern Atom a_NET_WM_WINDOW_TYPE_SPLASH;
extern Atom a_NET_WM_WINDOW_TYPE_DIALOG;
extern Atom a_NET_WM_WINDOW_TYPE_NORMAL;

extern Atom a_NET_WM_DESKTOP;
extern Atom a_NET_WM_NAME;
extern Atom a_NET_WM_VISIBLE_NAME;
extern Atom a_NET_WM_STRUT;
extern Atom a_NET_WM_STRUT_PARTIAL;
extern Atom a_NET_WM_ICON;
extern Atom a_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR;


extern xconf_enum allign_enum[];
extern xconf_enum edge_enum[];
extern xconf_enum widthtype_enum[];
extern xconf_enum heighttype_enum[];
extern xconf_enum bool_enum[];
extern xconf_enum pos_enum[];
extern xconf_enum layer_enum[];

extern int verbose;
extern gint force_quit;
extern FbEv *fbev;
extern GtkIconTheme *icon_theme;
#define FBPANEL_WIN(win)  gdk_window_lookup(win)
void panel_set_wm_strut(panel *p);

gchar *panel_get_profile(void);
gchar *panel_get_profile_file(void);

void ah_start(panel *p);
void ah_stop(panel *p);

#endif
