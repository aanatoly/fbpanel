/* fb-panel.c
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#include "fb-panel.h"

//#define DEBUGPRN
#include "dbg.h"


static void fb_panel_class_init(FbPanelClass *class);
static void fb_panel_init(FbPanel *self);
static void fb_panel_dispose(GObject *g_object);
static void fb_panel_finalize(GObject *g_object);
static void fb_panel_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec);
static void fb_panel_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec);
static void fb_panel_constructed(GObject *g_object);
static void fb_panel_size_request(GtkWidget *gtk_widget, GtkRequisition *requisition);
static void fb_panel_size_allocate(GtkWidget *gtk_widget, GtkAllocation *alloc);
static void fb_panel_button_press_event(GtkWidget *gtk_widget, GdkEventButton *event);
static gint fb_panel_configure_event(GtkWidget *gtk_widget, GdkEventConfigure *event);
static void fb_panel_map(GtkWidget *gtk_widget);
int fb_panel_command(FbPanel *self, gchar *cmd);
RbPanel *fb_panel_new(gchar *profile);


#define SELF(o) FbPanel *self G_GNUC_UNUSED = FB_PANEL(o)

static GtkWindowClass *parent_class = NULL;

enum {
    PROP_0,
    PROP_PROFILE,
    PROP_ALIGN,
    PROP_EDGE,
    PROP_WIDTH_TYPE,
    PROP_WIDTH,
    PROP_LAST
};

G_DEFINE_TYPE (FbPanel, fb_panel, GTK_TYPE_WINDOW
)


static void
fb_panel_class_init(FbPanelClass *class)
{
    parent_class = g_type_class_ref(GTK_TYPE_WINDOW);

    GObjectClass *g_object_class = G_OBJECT_CLASS(class);
    g_object_class->dispose = fb_panel_dispose;
    g_object_class->finalize = fb_panel_finalize;
    g_object_class->get_property = fb_panel_get_property;
    g_object_class->set_property = fb_panel_set_property;
    g_object_class->constructed = fb_panel_constructed;

    GtkWidgetClass *gtk_widget_class = GTK_WIDGET_CLASS(class);
    gtk_widget_class->size_request = fb_panel_size_request;
    gtk_widget_class->size_allocate = fb_panel_size_allocate;
    gtk_widget_class->button_press_event = fb_panel_button_press_event;
    gtk_widget_class->configure_event = fb_panel_configure_event;
    gtk_widget_class->map = fb_panel_map;

    g_object_class_install_property(g_object_class, PROP_PROFILE,
        g_param_spec_string("profile", "Short desc", "Long desc",
            NULL,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property(g_object_class, PROP_ALIGN,
        g_param_spec_enum("align",
            "Short desc", "Long desc",
            FB_TYPE_ALIGN_TYPE,
            FB_ALIGN_CENTER,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_EDGE,
        g_param_spec_enum("edge",
            "Short desc", "Long desc",
            FB_TYPE_EDGE_TYPE,
            FB_EDGE_BOTTOM,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_WIDTH_TYPE,
        g_param_spec_enum("width-type",
            "Short desc", "Long desc",
            FB_TYPE_WIDTH_TYPE,
            FB_WIDTH_PRECENT,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_WIDTH,
        g_param_spec_int("width", "Short desc", "Long desc",
            1, 3000, 80,
            G_PARAM_READWRITE));
}


static void
fb_panel_init(FbPanel *self)
{
    self->align = FB_ALIGN_CENTER;
    self->edge = FB_EDGE_BOTTOM;
    self->width_type = FB_WIDTH_PRECENT;
    self->width = 80;
}


static void
fb_panel_dispose(GObject *g_object)
{
    SELF(g_object);

    if (self->layout) {
        g_object_unref(self->layout);
        self->layout = NULL;
    }
    if (self->bg_box) {
        g_object_unref(self->bg_box);
        self->bg_box = NULL;
    }
    if (self->plugins_box) {
        g_object_unref(self->plugins_box);
        self->plugins_box = NULL;
    }
    if (self->menu) {
        g_object_unref(self->menu);
        self->menu = NULL;
    }
    G_OBJECT_CLASS(parent_class)->dispose(g_object);
}


static void
fb_panel_finalize(GObject *g_object)
{
    SELF(g_object);

    g_free(self->profile);
    G_OBJECT_CLASS(parent_class)->finalize(g_object);
}


static void
fb_panel_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec)
{
    SELF(g_object);
    switch (param_id) {
    case PROP_PROFILE:
        g_value_set_string(value, self->profile);
        break;
    case PROP_ALIGN:
        g_value_set_enum(value, self->align);
        break;
    case PROP_EDGE:
        g_value_set_enum(value, self->edge);
        break;
    case PROP_WIDTH_TYPE:
        g_value_set_enum(value, self->width_type);
        break;
    case PROP_WIDTH:
        g_value_set_int(value, self->width);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
}


static void
fb_panel_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec)
{
    SELF(g_object);
    switch (param_id) {
    case PROP_PROFILE:
        g_free(self->profile);
        self->profile = g_value_dup_string(value);
        break;
    case PROP_ALIGN:
        self->align = g_value_get_enum(value);
        break;
    case PROP_EDGE:
        self->edge = g_value_get_enum(value);
        break;
    case PROP_WIDTH_TYPE:
        self->width_type = g_value_get_enum(value);
        break;
    case PROP_WIDTH:
        self->width = g_value_get_int(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
}


static void
fb_panel_constructed(GObject *g_object)
{
    SELF(g_object);

    G_OBJECT_CLASS(parent_class)->constructed(g_object);
}


static void
fb_panel_size_request(GtkWidget *gtk_widget, GtkRequisition *requisition)
{
    SELF(gtk_widget);

    GTK_WIDGET_CLASS(parent_class)->size_request(gtk_widget, requisition);
}


static void
fb_panel_size_allocate(GtkWidget *gtk_widget, GtkAllocation *alloc)
{
    SELF(gtk_widget);

    GTK_WIDGET_CLASS(parent_class)->size_allocate(gtk_widget, alloc);
}


static void
fb_panel_button_press_event(GtkWidget *gtk_widget, GdkEventButton *event)
{
    SELF(gtk_widget);

    GTK_WIDGET_CLASS(parent_class)->button_press_event(gtk_widget, event);
}


static gint
fb_panel_configure_event(GtkWidget *gtk_widget, GdkEventConfigure *event)
{
    SELF(gtk_widget);

    GTK_WIDGET_CLASS(parent_class)->configure_event(gtk_widget, event);
    return 0;
}


static void
fb_panel_map(GtkWidget *gtk_widget)
{
    SELF(gtk_widget);

    GTK_WIDGET_CLASS(parent_class)->map(gtk_widget);
}


int
fb_panel_command(FbPanel *self, gchar *cmd)
{
    return 0;
}


RbPanel *
fb_panel_new(gchar *profile)
{
    return g_object_new(FB_TYPE_PANEL, "profile", profile, NULL);
}
