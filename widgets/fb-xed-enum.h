/* fb-xed-enum.h
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#ifndef __FB_XED_ENUM__
#define __FB_XED_ENUM__

#include <glib.h>
#include "fb-xed.h"


G_BEGIN_DECLS


typedef struct _FbXedEnum FbXedEnum;
typedef struct _FbXedEnumClass FbXedEnumClass;


#define FB_TYPE_XED_ENUM \
    (fb_xed_enum_get_type())
#define FB_XED_ENUM(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), FB_TYPE_XED_ENUM, FbXedEnum))
#define FB_XED_ENUM_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), FB_TYPE_XED_ENUM, FbXedEnumClass))
#define FB_IS_XED_ENUM(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), FB_TYPE_XED_ENUM))
#define FB_IS_XED_ENUM_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE((klass), FB_TYPE_XED_ENUM))
#define FB_XED_ENUM_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), FB_TYPE_XED_ENUM, FbXedEnumClass))

struct _FbXedEnum {
    FbXed parent;
};


struct _FbXedEnumClass {
    FbXedClass parent;
};


GType fb_xed_enum_get_type(void) G_GNUC_CONST;

GtkWidget *fb_xed_enum_new(GObject *master, gchar *prop_name, GtkSizeGroup *size_group);

G_END_DECLS


#endif /* __FB_XED_ENUM__ */
