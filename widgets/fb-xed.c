/* fb-xed.c
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#include "fb-xed.h"

//#define DEBUGPRN
#include "dbg.h"


static void fb_xed_class_init(FbXedClass *class);
static void fb_xed_init(FbXed *self);
static void fb_xed_dispose(GObject *g_object);
static void fb_xed_finalize(GObject *g_object);
static void fb_xed_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec);
static void fb_xed_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec);
static void fb_xed_constructed(GObject *g_object);
void fb_xed_load(FbXed *self);
static void fb_xed_load_real(FbXed *self);
void fb_xed_build(FbXed *self);
static void fb_xed_build_real(FbXed *self);
void fb_xed_add_label(FbXed *self);
GtkWidget *fb_xed_new(GObject *master, gchar *prop_name, GtkSizeGroup *size_group);
static void fb_xed_master_prop_changed(GObject *conf, GParamSpec *pspec, gpointer data);

#define SELF(o) FbXed *self G_GNUC_UNUSED = FB_XED(o)

static GtkHBoxClass *parent_class = NULL;

enum {
    PROP_0,
    PROP_MASTER,
    PROP_PROP_NAME,
    PROP_SIZE_GROUP,
    PROP_LAST
};

G_DEFINE_TYPE (FbXed, fb_xed, GTK_TYPE_HBOX
)


static void
fb_xed_class_init(FbXedClass *class)
{
    parent_class = g_type_class_ref(GTK_TYPE_HBOX);

    ENTER;
    GObjectClass *g_object_class = G_OBJECT_CLASS(class);
    g_object_class->dispose = fb_xed_dispose;
    g_object_class->finalize = fb_xed_finalize;
    g_object_class->get_property = fb_xed_get_property;
    g_object_class->set_property = fb_xed_set_property;
    g_object_class->constructed = fb_xed_constructed;

    FbXedClass *fb_xed_class = FB_XED_CLASS(class);
    fb_xed_class->load = fb_xed_load_real;
    fb_xed_class->build = fb_xed_build_real;

    g_object_class_install_property(g_object_class, PROP_MASTER,
        g_param_spec_object("master", "Master", "Long desc",
            G_TYPE_OBJECT,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property(g_object_class, PROP_PROP_NAME,
        g_param_spec_string("prop-name", "Prop Name", "Long desc",
            NULL,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property(g_object_class, PROP_SIZE_GROUP,
        g_param_spec_object("size-group", "Size Group", "Long desc",
            GTK_TYPE_SIZE_GROUP,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void
fb_xed_init(FbXed *self)
{
    ENTER;
    RET();
}


static void
fb_xed_dispose(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    if (self->master) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(self->master),
            fb_xed_master_prop_changed, g_object);
        g_object_unref(self->master);
        self->master = NULL;
    }
    if (self->size_group) {
        g_object_unref(self->size_group);
        self->size_group = NULL;
    }
    G_OBJECT_CLASS(parent_class)->dispose(g_object);
    RET();
}


static void
fb_xed_finalize(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    g_free(self->prop_name);
    G_OBJECT_CLASS(parent_class)->finalize(g_object);
    RET();
}


static void
fb_xed_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec)
{
    SELF(g_object);

    ENTER;
    switch (param_id) {
    case PROP_MASTER:
        g_value_set_object(value, (GObject *)self->master);
        break;
    case PROP_PROP_NAME:
        g_value_set_string(value, self->prop_name);
        break;
    case PROP_SIZE_GROUP:
        g_value_set_object(value, (GObject *)self->size_group);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
    RET();
}


static void
fb_xed_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec)
{
    SELF(g_object);

    ENTER;
    switch (param_id) {
    case PROP_MASTER:
        if (self->master)
            g_object_unref(self->master);
        self->master = g_value_get_object(value);
        if (self->master)
            g_object_ref(self->master);
        break;
    case PROP_PROP_NAME:
        g_free(self->prop_name);
        self->prop_name = g_value_dup_string(value);
        break;
    case PROP_SIZE_GROUP:
        if (self->size_group)
            g_object_unref(self->size_group);
        self->size_group = g_value_get_object(value);
        if (self->size_group)
            g_object_ref(self->size_group);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
    RET();
}

static void
fb_xed_master_prop_changed(GObject *conf,
    GParamSpec *pspec,
    gpointer data)
{
    SELF(data);

    DBG("master prop '%s' changed\n",  g_param_spec_get_name(pspec));
    fb_xed_load(self);
}


static void
fb_xed_constructed(GObject *g_object)
{
    SELF(g_object);
    gchar buf[100];
    //GtkWidget *label;

    GType gt;

    ENTER;
    g_object_set(g_object, "spacing", 8, NULL);
    if (!self->prop_name)
        RET();
    gt = G_OBJECT_TYPE(self->master);
    self->ps = g_object_class_find_property(
        G_OBJECT_CLASS(g_type_class_ref(gt)),
        self->prop_name);
    if (!self->ps) {
        ERR("object %s has no '%s' prop\n", g_type_name(gt), self->prop_name);
        RET();
    }

    snprintf(buf, sizeof(buf), "notify::%s", self->prop_name);
    g_signal_connect(self->master, buf,
        G_CALLBACK(fb_xed_master_prop_changed), self);
    fb_xed_build(self);
    fb_xed_load(self);
    RET();
}


void
fb_xed_load(FbXed *self)
{
    ENTER;
    self->in_update = TRUE;
    FB_XED_GET_CLASS(self)->load(self);
    self->in_update = FALSE;
    RET();
}


static void
fb_xed_load_real(FbXed *self)
{
    gint val = -1;
    gchar buf[100];
    ENTER;
    g_object_get(self->master, self->prop_name, &val, NULL);
    DBG("load : %s = %d\n", self->prop_name, val);
    snprintf(buf, sizeof(buf), "%d", val);
    gtk_label_set_text(GTK_LABEL(self->editor), buf);
    RET();
}


void
fb_xed_build(FbXed *self)
{
    ENTER;
    FB_XED_GET_CLASS(self)->build(self);
    gtk_box_pack_start(GTK_BOX(self), gtk_label_new(NULL), TRUE, TRUE, 0);
    RET();
}


static void
fb_xed_build_real(FbXed *self)
{
    ENTER;
    fb_xed_add_label(self);
    self->editor = gtk_label_new("val");
    gtk_box_pack_start(GTK_BOX(self), self->editor, FALSE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(self));
    RET();
}


void
fb_xed_add_label(FbXed *self)
{
    ENTER;
    self->label = gtk_label_new(g_param_spec_get_nick(self->ps));
    gtk_misc_set_alignment(GTK_MISC(self->label), 0.0, 0.5);
    if (self->size_group)
        gtk_size_group_add_widget(self->size_group, self->label);
    gtk_box_pack_start(GTK_BOX(self), self->label, FALSE, TRUE, 0);

}


GtkWidget *
fb_xed_new(GObject *master, gchar *prop_name, GtkSizeGroup *size_group)
{
    return g_object_new(FB_TYPE_XED, "master", master, "prop_name", prop_name, "size_group", size_group, NULL);
}
