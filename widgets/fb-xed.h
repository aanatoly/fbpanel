/* fb-xed.h
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#ifndef __FB_XED__
#define __FB_XED__

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "enums.h"


G_BEGIN_DECLS


typedef struct _FbXed FbXed;
typedef struct _FbXedClass FbXedClass;


#define FB_TYPE_XED \
    (fb_xed_get_type())
#define FB_XED(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), FB_TYPE_XED, FbXed))
#define FB_XED_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), FB_TYPE_XED, FbXedClass))
#define FB_IS_XED(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), FB_TYPE_XED))
#define FB_IS_XED_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE((klass), FB_TYPE_XED))
#define FB_XED_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), FB_TYPE_XED, FbXedClass))

struct _FbXed {
    GtkHBox parent;

    // start instance vars
    gboolean in_update;
    GParamSpec *ps;
    GtkWidget *label;
    GtkWidget *editor;
    GObject *master;
    gchar *prop_name;
    GtkSizeGroup *size_group;
    // end instance vars
};


struct _FbXedClass {
    GtkHBoxClass parent;
    void (* load)(FbXed *self);
    void (* build)(FbXed *self);
};


GType fb_xed_get_type(void) G_GNUC_CONST;

void fb_xed_load(FbXed *self);
void fb_xed_build(FbXed *self);
void fb_xed_add_label(FbXed *self);
GtkWidget *fb_xed_new(GObject *master, gchar *prop_name, GtkSizeGroup *size_group);

G_END_DECLS


#endif /* __FB_XED__ */
