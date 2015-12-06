/* fb-xed-color.h
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#ifndef __FB_XED_COLOR__
#define __FB_XED_COLOR__

#include <glib.h>
#include "fb-xed.h"


G_BEGIN_DECLS


typedef struct _FbXedColor FbXedColor;
typedef struct _FbXedColorClass FbXedColorClass;


#define FB_TYPE_XED_COLOR \
    (fb_xed_color_get_type())
#define FB_XED_COLOR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), FB_TYPE_XED_COLOR, FbXedColor))
#define FB_XED_COLOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), FB_TYPE_XED_COLOR, FbXedColorClass))
#define FB_IS_XED_COLOR(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), FB_TYPE_XED_COLOR))
#define FB_IS_XED_COLOR_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE((klass), FB_TYPE_XED_COLOR))
#define FB_XED_COLOR_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), FB_TYPE_XED_COLOR, FbXedColorClass))

struct _FbXedColor {
    FbXed parent;

    // start instance vars
    gchar *prop_pfx;
    // end instance vars
};


struct _FbXedColorClass {
    FbXedClass parent;
};


GType fb_xed_color_get_type(void) G_GNUC_CONST;

GtkWidget *fb_xed_color_new(GObject *master, gchar *prop_pfx, GtkSizeGroup *size_group);

G_END_DECLS


#endif /* __FB_XED_COLOR__ */
