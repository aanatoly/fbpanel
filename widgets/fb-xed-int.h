/* fb-xed-int.h
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#ifndef __FB_XED_INT__
#define __FB_XED_INT__

#include <glib.h>
#include "fb-xed.h"


G_BEGIN_DECLS


typedef struct _FbXedInt FbXedInt;
typedef struct _FbXedIntClass FbXedIntClass;


#define FB_TYPE_XED_INT \
    (fb_xed_int_get_type())
#define FB_XED_INT(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), FB_TYPE_XED_INT, FbXedInt))
#define FB_XED_INT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), FB_TYPE_XED_INT, FbXedIntClass))
#define FB_IS_XED_INT(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), FB_TYPE_XED_INT))
#define FB_IS_XED_INT_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE((klass), FB_TYPE_XED_INT))
#define FB_XED_INT_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), FB_TYPE_XED_INT, FbXedIntClass))

struct _FbXedInt {
    FbXed parent;
};


struct _FbXedIntClass {
    FbXedClass parent;
};


GType fb_xed_int_get_type(void) G_GNUC_CONST;

GtkWidget *fb_xed_int_new(GObject *master, gchar *prop_name, GtkSizeGroup *size_group);

G_END_DECLS


#endif /* __FB_XED_INT__ */
