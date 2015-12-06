/* fb-xed-int.c
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#include "fb-xed-int.h"

//#define DEBUGPRN
#include "dbg.h"


static void fb_xed_int_class_init(FbXedIntClass *class);
static void fb_xed_int_init(FbXedInt *self);
static void fb_xed_int_dispose(GObject *g_object);
static void fb_xed_int_finalize(GObject *g_object);
static void fb_xed_int_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec);
static void fb_xed_int_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec);
static void fb_xed_int_constructed(GObject *g_object);
static void fb_xed_int_load(FbXed *fb_xed);
static void fb_xed_int_build(FbXed *fb_xed);
GtkWidget *fb_xed_int_new(GObject *master, gchar *prop_name, GtkSizeGroup *size_group);


#define SELF(o) FbXedInt *self G_GNUC_UNUSED = FB_XED_INT(o)

static FbXedClass *parent_class = NULL;

G_DEFINE_TYPE (FbXedInt, fb_xed_int, FB_TYPE_XED
)


static void
fb_xed_int_class_init(FbXedIntClass *class)
{
    parent_class = g_type_class_ref(FB_TYPE_XED);

    ENTER;
    GObjectClass *g_object_class = G_OBJECT_CLASS(class);
    g_object_class->dispose = fb_xed_int_dispose;
    g_object_class->finalize = fb_xed_int_finalize;
    g_object_class->get_property = fb_xed_int_get_property;
    g_object_class->set_property = fb_xed_int_set_property;
    g_object_class->constructed = fb_xed_int_constructed;

    FbXedClass *fb_xed_class = FB_XED_CLASS(class);
    fb_xed_class->load = fb_xed_int_load;
    fb_xed_class->build = fb_xed_int_build;
}


static void
fb_xed_int_init(FbXedInt *self)
{
    ENTER;
    RET();
}


static void
fb_xed_int_dispose(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    G_OBJECT_CLASS(parent_class)->dispose(g_object);
    RET();
}


static void
fb_xed_int_finalize(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    G_OBJECT_CLASS(parent_class)->finalize(g_object);
    RET();
}


static void
fb_xed_int_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec)
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
fb_xed_int_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec)
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
fb_xed_int_constructed(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    G_OBJECT_CLASS(parent_class)->constructed(g_object);
    RET();
}


static void
fb_xed_int_load(FbXed *fb_xed)
{
    SELF(fb_xed);
    gint val;

    ENTER;
    g_object_get(G_OBJECT(fb_xed->master), fb_xed->prop_name, &val, NULL);
    DBG("load : %s = %d\n", FB_XED(self)->prop_name, val);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(fb_xed->editor), val);
    RET();
}


static void
fb_xed_int_value_changed(GtkSpinButton *sb, gpointer data)
{
    SELF(data);
    gint val;

    ENTER;
    if (FB_XED(self)->in_update) {
        DBG("in update\n");
        return;
    }
    val = gtk_spin_button_get_value_as_int(sb);
    DBG("new val: %d\n", val);
    g_object_set(FB_XED(self)->master, FB_XED(self)->prop_name, val, NULL);
    RET();
}


static void
fb_xed_int_build(FbXed *fb_xed)
{
    SELF(fb_xed);
    GParamSpecInt *ps = (GParamSpecInt *) fb_xed->ps;

    ENTER;
    fb_xed_add_label(fb_xed);
    fb_xed->editor = gtk_spin_button_new_with_range(
        ps->minimum,
        ps->maximum,
        //FIXME: make step dependson min and max
        1);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(fb_xed->editor), 0);
    g_signal_connect(G_OBJECT(fb_xed->editor), "value-changed",
        (GCallback) fb_xed_int_value_changed, self);

    gtk_box_pack_start(GTK_BOX(self), fb_xed->editor, FALSE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(self));
    RET();
}


GtkWidget *
fb_xed_int_new(GObject *master, gchar *prop_name, GtkSizeGroup *size_group)
{
    return g_object_new(FB_TYPE_XED_INT, "master", master, "prop_name", prop_name, "size_group", size_group, NULL);
}
