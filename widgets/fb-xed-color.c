/* fb-xed-color.c
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#include "fb-xed-color.h"

//#define DEBUGPRN
#include "dbg.h"


static void fb_xed_color_class_init(FbXedColorClass *class);
static void fb_xed_color_init(FbXedColor *self);
static void fb_xed_color_dispose(GObject *g_object);
static void fb_xed_color_finalize(GObject *g_object);
static void fb_xed_color_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec);
static void fb_xed_color_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec);
static void fb_xed_color_constructed(GObject *g_object);
static void fb_xed_color_load(FbXed *fb_xed);
static void fb_xed_color_build(FbXed *fb_xed);
GtkWidget *fb_xed_color_new(GObject *master, gchar *prop_pfx, GtkSizeGroup *size_group);


#define SELF(o) FbXedColor *self G_GNUC_UNUSED = FB_XED_COLOR(o)

static FbXedClass *parent_class = NULL;

enum {
    PROP_0,
    PROP_PROP_PFX,
    PROP_LAST
};

G_DEFINE_TYPE (FbXedColor, fb_xed_color, FB_TYPE_XED
)


static void
fb_xed_color_class_init(FbXedColorClass *class)
{
    parent_class = g_type_class_ref(FB_TYPE_XED);

    ENTER;
    GObjectClass *g_object_class = G_OBJECT_CLASS(class);
    g_object_class->dispose = fb_xed_color_dispose;
    g_object_class->finalize = fb_xed_color_finalize;
    g_object_class->get_property = fb_xed_color_get_property;
    g_object_class->set_property = fb_xed_color_set_property;
    g_object_class->constructed = fb_xed_color_constructed;

    FbXedClass *fb_xed_class = FB_XED_CLASS(class);
    fb_xed_class->load = fb_xed_color_load;
    fb_xed_class->build = fb_xed_color_build;

    g_object_class_install_property(g_object_class, PROP_PROP_PFX,
        g_param_spec_string("prop-pfx", "Prop Pfx", "Long desc",
            NULL,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void
fb_xed_color_init(FbXedColor *self)
{
    ENTER;
    RET();
}


static void
fb_xed_color_dispose(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    g_signal_handlers_disconnect_by_func(FB_XED(self)->master,
        fb_xed_load, g_object);
    G_OBJECT_CLASS(parent_class)->dispose(g_object);
    RET();
}


static void
fb_xed_color_finalize(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    g_free(self->prop_pfx);
    G_OBJECT_CLASS(parent_class)->finalize(g_object);
    RET();
}


static void
fb_xed_color_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec)
{
    SELF(g_object);

    ENTER;
    switch (param_id) {
    case PROP_PROP_PFX:
        g_value_set_string(value, self->prop_pfx);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
    RET();
}


static void
fb_xed_color_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec)
{
    SELF(g_object);

    ENTER;
    switch (param_id) {
    case PROP_PROP_PFX:
        g_free(self->prop_pfx);
        self->prop_pfx = g_value_dup_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
    RET();
}


static void
fb_xed_color_constructed(GObject *g_object)
{
    SELF(g_object);
    FbXed *fb_xed = (FbXed *) self;
    gchar buf[100];

    ENTER;
    snprintf(buf, sizeof(buf), "notify::%s-color", self->prop_pfx);
    g_signal_connect_swapped(fb_xed->master, buf,
        G_CALLBACK(fb_xed_load), self);
    snprintf(buf, sizeof(buf), "notify::%s-alpha", self->prop_pfx);
    g_signal_connect_swapped(fb_xed->master, buf,
        G_CALLBACK(fb_xed_load), self);
    fb_xed_build(fb_xed);
    fb_xed_load(fb_xed);
    RET();
}


static void
fb_xed_color_load(FbXed *fb_xed)
{
    SELF(fb_xed);
    guint alpha;
    GdkColor *color;
    gchar buf[100];
    GtkColorButton *cb = (GtkColorButton *) fb_xed->editor;

    ENTER;
    snprintf(buf, sizeof(buf), "%s-color", self->prop_pfx);
    g_object_get(FB_XED(self)->master, buf, &color, NULL);
    if (!color)
        RET();
    color->pixel = 0;
    gtk_color_button_set_color(cb, color);
    snprintf(buf, sizeof(buf), "%s-alpha", self->prop_pfx);
    g_object_get(FB_XED(self)->master, buf, &alpha, NULL);
    gtk_color_button_set_alpha(cb, alpha << 8);
    DBG("Load color (prop): [%d] %x %x %x\n", alpha,
        color->red, color->green, color->blue);
    RET();
}


static void
fb_xed_color_value_changed(GtkColorButton *cb, gpointer data)
{
    SELF(data);
    guint alpha;
    GdkColor color;
    gchar buf[100];

    ENTER;
    if (FB_XED(self)->in_update) {
        DBG("in update\n");
        RET();
    }
    gtk_color_button_get_color(cb, &color);
    color.pixel = 0;
    DBG("New color (api): %x %x %x\n", color.red, color.green, color.blue);
    alpha = gtk_color_button_get_alpha(cb) >> 8;
    DBG("New alpha (api): %d\n", alpha);

    snprintf(buf, sizeof(buf), "%s-color", self->prop_pfx);
    g_object_set(FB_XED(self)->master, buf, &color, NULL);
    snprintf(buf, sizeof(buf), "%s-alpha", self->prop_pfx);
    g_object_set(FB_XED(self)->master, buf, alpha, NULL);
    RET();
}


static void
fb_xed_color_build(FbXed *fb_xed)
{
    SELF(fb_xed);

    ENTER;
    fb_xed->label = gtk_label_new(self->prop_pfx);
    gtk_misc_set_alignment(GTK_MISC(fb_xed->label), 0.0, 0.5);
    if (fb_xed->size_group)
        gtk_size_group_add_widget(fb_xed->size_group, fb_xed->label);
    gtk_box_pack_start(GTK_BOX(self), fb_xed->label, FALSE, TRUE, 0);

    fb_xed->editor = gtk_color_button_new();
    g_signal_connect(G_OBJECT(fb_xed->editor), "color-set",
        (GCallback) fb_xed_color_value_changed, self);
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(fb_xed->editor), TRUE);
    gtk_box_pack_start(GTK_BOX(self), fb_xed->editor, FALSE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(self));
    RET();
}


GtkWidget *
fb_xed_color_new(GObject *master, gchar *prop_pfx, GtkSizeGroup *size_group)
{
    return g_object_new(FB_TYPE_XED_COLOR, "master", master, "prop_pfx", prop_pfx, "size_group", size_group, NULL);
}
