/* fb-panel.c
** Copyright 2015 by anatoly@fgmail.com
** Licence GPLv2
*/

#include "fb-panel.h"
#include <string.h>

#define DEBUGPRN
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
static gint fb_panel_button_press_event(GtkWidget *gtk_widget, GdkEventButton *event);
static gboolean fb_panel_scroll_event(GtkWidget *gtk_widget, GdkEventScroll *event);
static gint fb_panel_configure_event(GtkWidget *gtk_widget, GdkEventConfigure *event);
static void fb_panel_map(GtkWidget *gtk_widget);
int fb_panel_command(FbPanel *self, gchar *cmd);
GtkWidget *fb_panel_new(gchar *profile);
static void fb_panel_queue_layout(FbPanel *self);
static gboolean fb_panel_layout(gpointer data);

#define SELF(o) FbPanel *self G_GNUC_UNUSED = FB_PANEL(o)

static GtkWindowClass *parent_class = NULL;

enum {
    PROP_0,
    PROP_PROFILE,
    PROP_ALIGN,
    PROP_EDGE,
    PROP_WIDTH_TYPE,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_TRANSPARENT,
    PROP_TINT_COLOR,
    PROP_TINT_ALPHA,
    PROP_POS_X,
    PROP_POS_Y,
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
    gtk_widget_class->scroll_event = fb_panel_scroll_event;
    gtk_widget_class->configure_event = fb_panel_configure_event;
    gtk_widget_class->map = fb_panel_map;

    g_object_class_install_property(g_object_class, PROP_PROFILE,
        g_param_spec_string("profile", "Profile", "Long desc",
            NULL,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property(g_object_class, PROP_ALIGN,
        g_param_spec_enum("align",
            "Align", "Long desc",
            FB_TYPE_ALIGN_TYPE,
            FB_ALIGN_CENTER,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_EDGE,
        g_param_spec_enum("edge",
            "Edge", "Long desc",
            FB_TYPE_EDGE_TYPE,
            FB_EDGE_BOTTOM,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_WIDTH_TYPE,
        g_param_spec_enum("width-type",
            "Width Type", "Long desc",
            FB_TYPE_WIDTH_TYPE,
            FB_WIDTH_PERCENT,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_WIDTH,
        g_param_spec_int("width", "Width", "Long desc",
            1, 3000, 80,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_HEIGHT,
        g_param_spec_int("height", "Height", "Long desc",
            1, 3000, 80,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_TRANSPARENT,
        g_param_spec_boolean("transparent", "Transparent", "Long desc",
            TRUE,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_TINT_COLOR,
        g_param_spec_boxed("tint-color", "Tint Color", "Long desc",
            GDK_TYPE_COLOR,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_TINT_ALPHA,
        g_param_spec_int("tint-alpha", "Tint Alpha", "Long desc",
            1, 255, 80,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_POS_X,
        g_param_spec_int("pos-x", "Pos X", "Long desc",
            1, 5255, 80,
            G_PARAM_READWRITE));
    g_object_class_install_property(g_object_class, PROP_POS_Y,
        g_param_spec_int("pos-y", "Pos Y", "Long desc",
            1, 5255, 80,
            G_PARAM_READWRITE));
}


static void
fb_panel_init(FbPanel *self)
{
    ENTER;
    // start instance vars
    self->align = FB_ALIGN_CENTER;
    self->edge = FB_EDGE_BOTTOM;
    self->width_type = FB_WIDTH_PERCENT;
    self->width = 80;
    self->height = 80;
    self->transparent = TRUE;
    self->tint_alpha = 80;
    self->pos_x = 80;
    self->pos_y = 80;
    // end instance vars
    gtk_window_set_resizable(GTK_WINDOW(self), FALSE);
    gtk_window_set_wmclass(GTK_WINDOW(self), "panel", "fbpanel");
    gtk_window_set_title(GTK_WINDOW(self), "panel");
    gtk_window_set_position(GTK_WINDOW(self), GTK_WIN_POS_NONE);
    gtk_window_set_decorated(GTK_WINDOW(self), FALSE);
    gtk_window_set_accept_focus(GTK_WINDOW(self), FALSE);
    gtk_window_stick(GTK_WINDOW(self));
    RET();
}


static void
fb_panel_dispose(GObject *g_object)
{
    SELF(g_object);

    ENTER;
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
    RET();
}


static void
fb_panel_finalize(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    g_free(self->profile);
    G_OBJECT_CLASS(parent_class)->finalize(g_object);
    RET();
}


static void
fb_panel_get_property(GObject *g_object, guint param_id, GValue *value, GParamSpec *pspec)
{
    SELF(g_object);

    DBG("get prop '%s'\n", pspec->name);
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
    case PROP_HEIGHT:
        g_value_set_int(value, self->height);
        break;
    case PROP_TRANSPARENT:
        g_value_set_boolean(value, self->transparent);
        break;
    case PROP_TINT_COLOR:
        g_value_set_boxed(value, &self->tint_color);
        break;
    case PROP_TINT_ALPHA:
        g_value_set_int(value, self->tint_alpha);
        break;
    case PROP_POS_X:
        g_value_set_int(value, self->pos_x);
        break;
    case PROP_POS_Y:
        g_value_set_int(value, self->pos_y);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
    RET();
}


static void
fb_panel_set_property(GObject *g_object, guint param_id, const GValue *value, GParamSpec *pspec)
{
    SELF(g_object);
    gboolean repos = FALSE;

    DBG("set prop '%s'\n", pspec->name);
    switch (param_id) {
    case PROP_PROFILE:
        g_free(self->profile);
        self->profile = g_value_dup_string(value);
        break;
    case PROP_ALIGN:
        self->align = g_value_get_enum(value);
        repos = TRUE;
        break;
    case PROP_EDGE:
        self->edge = g_value_get_enum(value);
        repos = TRUE;
        break;
    case PROP_WIDTH_TYPE:
        self->width_type = g_value_get_enum(value);
        repos = TRUE;
        break;
    case PROP_WIDTH:
        self->width = g_value_get_int(value);
        repos = TRUE;
        break;
    case PROP_HEIGHT:
        self->height = g_value_get_int(value);
        repos = TRUE;
        break;
    case PROP_TRANSPARENT:
        self->transparent = g_value_get_boolean(value);
        break;
    case PROP_TINT_COLOR:
        self->tint_color = *(GdkColor *)g_value_dup_boxed(value);
        self->tint_color.pixel = 0;
        break;
    case PROP_TINT_ALPHA:
        self->tint_alpha = g_value_get_int(value);
        break;
    case PROP_POS_X:
        self->pos_x = g_value_get_int(value);
        repos = TRUE;
        break;
    case PROP_POS_Y:
        self->pos_y = g_value_get_int(value);
        repos = TRUE;
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(g_object, param_id, pspec);
        break;
    }
    if (repos)
        fb_panel_queue_layout(self);
    RET();
}


static void
fb_panel_constructed(GObject *g_object)
{
    SELF(g_object);

    ENTER;
    //G_OBJECT_CLASS(parent_class)->constructed(g_object);
    RET();
}


static void
fb_panel_size_request(GtkWidget *gtk_widget, GtkRequisition *requisition)
{
    SELF(gtk_widget);

    ENTER;
    requisition->width = self->req.width;
    requisition->height = self->req.height;
    DBG("req: %d %d\n", requisition->width, requisition->height);
    RET();
}


static void
fb_panel_size_allocate(GtkWidget *gtk_widget, GtkAllocation *alloc)
{
    SELF(gtk_widget);

    ENTER;
    DBG("alloc: %d %d\n", alloc->width, alloc->height);
    //GTK_WIDGET_CLASS(parent_class)->size_allocate(gtk_widget, alloc);
    RET();
}


static gint
fb_panel_button_press_event(GtkWidget *gtk_widget, GdkEventButton *event)
{
    SELF(gtk_widget);

    ENTER;
    GTK_WIDGET_CLASS(parent_class)->button_press_event(gtk_widget, event);
    RET(0);
}


static gboolean
fb_panel_scroll_event(GtkWidget *gtk_widget, GdkEventScroll *event)
{
    SELF(gtk_widget);

    ENTER;
    GTK_WIDGET_CLASS(parent_class)->scroll_event(gtk_widget, event);
    RET(0);
}


static gint
fb_panel_configure_event(GtkWidget *gtk_widget, GdkEventConfigure *event)
{
    SELF(gtk_widget);

    ENTER;
    self->pos.x = event->x;
    self->pos.y = event->y;
    self->pos.width = event->width;
    self->pos.height = event->height;
    DBG("pos (diff %d) %d, %d; geom %d, %d\n",
        memcmp(&self->pos, &self->req, sizeof(self->pos)),
        event->x, event->y,
        event->width, event->height);
    //GTK_WIDGET_CLASS(parent_class)->configure_event(gtk_widget, event);
    RET(0);
}


static void
fb_panel_map(GtkWidget *gtk_widget)
{
    SELF(gtk_widget);

    ENTER;
    GTK_WIDGET_CLASS(parent_class)->map(gtk_widget);
    RET();
}


int
fb_panel_command(FbPanel *self, gchar *cmd)
{
    return 0;
}


GtkWidget *
fb_panel_new(gchar *profile)
{
    return g_object_new(FB_TYPE_PANEL, "profile", profile, NULL);
}


static gboolean G_GNUC_UNUSED
movit(gpointer data)
{
    SELF(data);
    ENTER;
    gtk_window_resize(GTK_WINDOW(self), 40, 40);
    gtk_window_move(GTK_WINDOW(self), 20, 20);
    RET(FALSE);
}

static gboolean
fb_panel_layout(gpointer data)
{
    SELF(data);
    // FIXME: cache them
    //int scr_w = gdk_screen_width();
    //int scr_h = gdk_screen_height();
    gboolean qmove = FALSE, qresize = FALSE;

    ENTER;
    self->lid = 0;
    DBG("geom: x %d, y %d, w %d, h %d\n",
        self->req.x, self->req.y, self->req.width, self->req.height);
    if (self->pos_x != self->pos.x) {
        self->req.x = self->pos_x;
        qmove = TRUE;
    }
    if (self->pos_y != self->pos.y) {
        self->req.y = self->pos_y;
        qmove = TRUE;
    }
    if (self->width != self->pos.width) {
        self->req.width = self->width;
        qresize = TRUE;
    }
    if (self->height != self->pos.height) {
        self->req.height = self->height;
        qresize = TRUE;
    }
    DBG("geom: x %d, y %d, w %d, h %d\n",
        self->req.x, self->req.y, self->req.width, self->req.height);
    if (qmove)
        gtk_window_move(GTK_WINDOW(self), self->req.x, self->req.y);
    if (qresize)
        gtk_window_resize(GTK_WINDOW(self), self->req.width, self->req.height);

    //  g_timeout_add(300, movit, self);
    //set_horiz_slide(self, &a_y, &a_h);
    RET(FALSE);
}


static void
fb_panel_queue_layout(FbPanel *self)
{
    static gint64 ts, nts;

    nts = g_get_monotonic_time() / 1000;
    DBG("ts diff: %d\n", (int) (nts - ts));
    ts = nts;
    if (self->lid)
        return;
    self->lid = g_timeout_add(50, fb_panel_layout, self);
}
