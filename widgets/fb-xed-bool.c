/* fb-xed-bool.c
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#include "fb-xed-bool.h"

//#define DEBUGPRN
#include "dbg.h"


static void fb_xed_bool_class_init(FbXedBoolClass *class);
static void fb_xed_bool_init(FbXedBool *self);
static void fb_xed_bool_dispose(GObject *g_object);
static void fb_xed_bool_finalize(GObject *g_object);
static void fb_xed_bool_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec);
static void fb_xed_bool_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec);
static void fb_xed_bool_constructed(GObject *g_object);
static void fb_xed_bool_load(FbXed *fb_xed);
static void fb_xed_bool_build(FbXed *fb_xed);
GtkWidget *fb_xed_bool_new(GObject *master, gchar *prop_name);


#define SELF(o) FbXedBool *self G_GNUC_UNUSED = FB_XED_BOOL(o)

static FbXedClass *parent_class = NULL;

G_DEFINE_TYPE (FbXedBool, fb_xed_bool, FB_TYPE_XED
)


static void
fb_xed_bool_class_init(FbXedBoolClass *class)
{
    ENTER;
    parent_class = g_type_class_ref(FB_TYPE_XED);

    GObjectClass *g_object_class = G_OBJECT_CLASS(class);
    g_object_class->dispose = fb_xed_bool_dispose;
    g_object_class->finalize = fb_xed_bool_finalize;
    g_object_class->get_property = fb_xed_bool_get_property;
    g_object_class->set_property = fb_xed_bool_set_property;
    g_object_class->constructed = fb_xed_bool_constructed;

    FbXedClass *fb_xed_class = FB_XED_CLASS(class);
    fb_xed_class->load = fb_xed_bool_load;
    fb_xed_class->build = fb_xed_bool_build;
    RET();
}


static void
fb_xed_bool_init(FbXedBool *self)
{
    ENTER;
    RET();
}


static void
fb_xed_bool_dispose(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    G_OBJECT_CLASS(parent_class)->dispose(g_object);
    RET();
}


static void
fb_xed_bool_finalize(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    G_OBJECT_CLASS(parent_class)->finalize(g_object);
    RET();
}


static void
fb_xed_bool_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec)
{
    SELF(g_object);
    ENTER;
    switch (param_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
    RET();
}


static void
fb_xed_bool_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec)
{
    SELF(g_object);
    ENTER;
    switch (param_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
    RET();
}


static void
fb_xed_bool_constructed(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    G_OBJECT_CLASS(parent_class)->constructed(g_object);
    RET();
}


static void
fb_xed_bool_load(FbXed *fb_xed)
{
    SELF(fb_xed);
    gboolean val;

    ENTER;
    g_object_get(G_OBJECT(fb_xed->master), fb_xed->prop_name, &val, NULL);
    DBG("load : %s = %d\n", FB_XED(self)->prop_name, val);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fb_xed->editor), val);
    RET();
}


static void
fb_xed_bool_value_changed(GtkToggleButton *tb,
    gpointer data)
{
    SELF(data);
    gboolean val;

    ENTER;
    if (FB_XED(self)->in_update) {
        DBG("in update\n");
        return;
    }
    val = gtk_toggle_button_get_active(tb);
    DBG("new val: %d\n", val);
    g_object_set(FB_XED(self)->master, FB_XED(self)->prop_name, val, NULL);
    RET();
}


static void
fb_xed_bool_build(FbXed *fb_xed)
{
    SELF(fb_xed);

    ENTER;
    fb_xed_add_label(fb_xed);
    fb_xed->editor = gtk_check_button_new();
    gtk_widget_reparent(fb_xed->label, fb_xed->editor);
    //gtk_container_add(GTK_CONTAINER(fb_xed->editor), label);
    g_signal_connect(G_OBJECT(fb_xed->editor), "toggled",
        (GCallback) fb_xed_bool_value_changed, self);
    gtk_box_pack_start(GTK_BOX(self), fb_xed->editor, FALSE, TRUE, 0);
    RET();
}


GtkWidget *
fb_xed_bool_new(GObject *master, gchar *prop_name)
{
    ENTER;
    return g_object_new(FB_TYPE_XED_BOOL, "master", master, "prop_name", prop_name, NULL);
}
