/* fb-xed-bool.h
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#ifndef __FB_XED_BOOL__
#define __FB_XED_BOOL__

#include <glib.h>
#include "fb-xed.h"


G_BEGIN_DECLS


typedef struct _FbXedBool FbXedBool;
typedef struct _FbXedBoolClass FbXedBoolClass;


#define FB_TYPE_XED_BOOL \
    (fb_xed_bool_get_type())
#define FB_XED_BOOL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), FB_TYPE_XED_BOOL, FbXedBool))
#define FB_XED_BOOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), FB_TYPE_XED_BOOL, FbXedBoolClass))
#define FB_IS_XED_BOOL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), FB_TYPE_XED_BOOL))
#define FB_IS_XED_BOOL_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE((klass), FB_TYPE_XED_BOOL))
#define FB_XED_BOOL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), FB_TYPE_XED_BOOL, FbXedBoolClass))

struct _FbXedBool {
    FbXed parent;
};


struct _FbXedBoolClass {
    FbXedClass parent;
};


GType fb_xed_bool_get_type(void) G_GNUC_CONST;

GtkWidget *fb_xed_bool_new(GObject *master, gchar *prop_name);

G_END_DECLS


#endif /* __FB_XED_BOOL__ */
