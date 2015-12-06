/* fb-xed-enum.c
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#include "fb-xed-enum.h"

//#define DEBUGPRN
#include "dbg.h"


static void fb_xed_enum_class_init(FbXedEnumClass *class);
static void fb_xed_enum_init(FbXedEnum *self);
static void fb_xed_enum_dispose(GObject *g_object);
static void fb_xed_enum_finalize(GObject *g_object);
static void fb_xed_enum_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec);
static void fb_xed_enum_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec);
static void fb_xed_enum_constructed(GObject *g_object);
static void fb_xed_enum_load(FbXed *fb_xed);
static void fb_xed_enum_build(FbXed *fb_xed);
GtkWidget *fb_xed_enum_new(GObject *master, gchar *prop_name, GtkSizeGroup *size_group);


#define SELF(o) FbXedEnum *self G_GNUC_UNUSED = FB_XED_ENUM(o)

static FbXedClass *parent_class = NULL;

G_DEFINE_TYPE (FbXedEnum, fb_xed_enum, FB_TYPE_XED
)


static void
fb_xed_enum_class_init(FbXedEnumClass *class)
{
    ENTER;
    parent_class = g_type_class_ref(FB_TYPE_XED);

    GObjectClass *g_object_class = G_OBJECT_CLASS(class);
    g_object_class->dispose = fb_xed_enum_dispose;
    g_object_class->finalize = fb_xed_enum_finalize;
    g_object_class->get_property = fb_xed_enum_get_property;
    g_object_class->set_property = fb_xed_enum_set_property;
    g_object_class->constructed = fb_xed_enum_constructed;

    FbXedClass *fb_xed_class = FB_XED_CLASS(class);
    fb_xed_class->load = fb_xed_enum_load;
    fb_xed_class->build = fb_xed_enum_build;
    RET();
}


static void
fb_xed_enum_init(FbXedEnum *self)
{
    ENTER;
    RET();
}


static void
fb_xed_enum_dispose(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    G_OBJECT_CLASS(parent_class)->dispose(g_object);
    RET();
}


static void
fb_xed_enum_finalize(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    G_OBJECT_CLASS(parent_class)->finalize(g_object);
    RET();
}


static void
fb_xed_enum_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec)
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
fb_xed_enum_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec)
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
fb_xed_enum_constructed(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    G_OBJECT_CLASS(parent_class)->constructed(g_object);
    RET();
}


static void
fb_xed_enum_load(FbXed *fb_xed)
{
    SELF(fb_xed);
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint val, val2;

    ENTER;
    g_object_get(fb_xed->master, fb_xed->prop_name, &val, NULL);
    DBG("load : %s = %d\n", FB_XED(self)->prop_name, val);
    model = gtk_combo_box_get_model(GTK_COMBO_BOX(fb_xed->editor));
    if (!gtk_tree_model_get_iter_first(model, &iter)) {
        ERR("xed-enum %s has no model or empty\n", fb_xed->prop_name);
        return;
    }
    do {
        gtk_tree_model_get(model, &iter, 0, &val2, -1);
        if (val == val2) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(fb_xed->editor), &iter);
            RET();
        }
    } while (gtk_tree_model_iter_next(model, &iter));
    ERR("xed-enum %s can't find value %d\n", fb_xed->prop_name, val);
    RET();
}


static void
fb_xed_enum_value_changed(GtkComboBox *cb,
    gpointer data)
{
    SELF(data);
    FbXed *fb_xed = (FbXed *) self;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint val = -1;

    ENTER;
    if (FB_XED(self)->in_update) {
        DBG("in update\n");
        return;
    }
    model = gtk_combo_box_get_model(cb);
    if (!gtk_combo_box_get_active_iter(cb, &iter)) {
        ERR("xed-enum %s has no active iter, while should\n",
            fb_xed->prop_name);
        RET();
    }
    gtk_tree_model_get(model, &iter, 0, &val, -1);
    DBG("new val: %d\n", val);
    g_object_set(fb_xed->master, fb_xed->prop_name, val, NULL);
    RET();
}


static void
fb_xed_enum_build(FbXed *fb_xed)
{
    SELF(fb_xed);
    // GParamSpecInt *ps = (GParamSpecInt *) fb_xed->ps;
    GtkListStore *store;
    GtkCellRenderer *cr;

    ENTER;
    fb_xed_add_label(fb_xed);
    store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
    if (1) {
        GParamSpecEnum *ps = (GParamSpecEnum *)fb_xed->ps;
        GEnumClass *ec = ps->enum_class;
        GEnumValue *ev;
        int i;

        for (i = 0; i < ec->n_values; i++) {
            ev = &ec->values[i];
            DBGE(" '%s'", ev->value_nick);
            gtk_list_store_insert_with_values(store, NULL, -1,
                0, ev->value,
                1, ev->value_nick,
                -1);
        }
    }
    fb_xed->editor = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    cr = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fb_xed->editor), cr, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(fb_xed->editor), cr,
        "text", 1,
        NULL);
    g_signal_connect(G_OBJECT(fb_xed->editor), "changed",
        (GCallback) fb_xed_enum_value_changed, self);

    //gtk_combo_box_set_active(GTK_COMBO_BOX(fb_xed->editor), 0);
    gtk_box_pack_start(GTK_BOX(self), fb_xed->editor, FALSE, TRUE, 0);
    RET();
}


GtkWidget *
fb_xed_enum_new(GObject *master, gchar *prop_name, GtkSizeGroup *size_group)
{
    return g_object_new(FB_TYPE_XED_ENUM, "master", master, "prop_name", prop_name, "size_group", size_group, NULL);
}
