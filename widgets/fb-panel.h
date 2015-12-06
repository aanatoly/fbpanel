/* fb-panel.h
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#ifndef __FB_PANEL__
#define __FB_PANEL__

#include <glib.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>


G_BEGIN_DECLS


typedef struct _FbPanel FbPanel;
typedef struct _FbPanelClass FbPanelClass;


#define FB_TYPE_PANEL \
    (fb_panel_get_type())
#define FB_PANEL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), FB_TYPE_PANEL, FbPanel))
#define FB_PANEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), FB_TYPE_PANEL, FbPanelClass))
#define FB_IS_PANEL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), FB_TYPE_PANEL))
#define FB_IS_PANEL_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE((klass), FB_TYPE_PANEL))
#define FB_PANEL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), FB_TYPE_PANEL, FbPanelClass))

struct _FbPanel {
    GtkWindow parent;

    Gtkwidget *layout;
    Gtkwidget *bg_box;
    Gtkwidget *plugins_box;
    Gtkwidget *menu;
    gchar *profile;
    FbAlignType align;
    FbEdgeType edge;
    FbWidthType width_type;
    gint width;
};


struct _FbPanelClass {
    GtkWindowClass parent;
};


GType fb_panel_get_type(void) G_GNUC_CONST;

int fb_panel_command(FbPanel *self, gchar *cmd);
RbPanel *fb_panel_new(gchar *profile);

G_END_DECLS


#endif /* __FB_PANEL__ */
