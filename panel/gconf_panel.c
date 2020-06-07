
#include "gconf.h"
#include "panel.h"

//#define DEBUGPRN
#include "dbg.h"

static GtkWidget *dialog;
static GtkWidget *width_spin, *width_opt;
static GtkWidget *xmargin_spin, *ymargin_spin;
static GtkWidget *allign_opt;

static gconf_block *gl_block;
static gconf_block *geom_block;
static gconf_block *prop_block;
static gconf_block *effects_block;
static gconf_block *color_block;
static gconf_block *corner_block;
static gconf_block *layer_block;
static gconf_block *ah_block;

#define INDENT_2 25


GtkWidget *mk_tab_plugins(xconf *xc);

/*********************************************************
 * panel effects
 *********************************************************/
static void
effects_changed(gconf_block *b)
{
    int i;

    ENTER;
    XCG(b->data, "transparent", &i, enum, bool_enum);
    gtk_widget_set_sensitive(color_block->main, i);
    XCG(b->data, "roundcorners", &i, enum, bool_enum);
    gtk_widget_set_sensitive(corner_block->main, i);
    XCG(b->data, "autohide", &i, enum, bool_enum);
    gtk_widget_set_sensitive(ah_block->main, i);

    RET();
}


static void
mk_effects_block(xconf *xc)
{
    GtkWidget *w;

    ENTER;

    /* label */
    w = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(w), 0, 0.5);
    gtk_label_set_markup(GTK_LABEL(w), _("<b>Visual Effects</b>"));
    gconf_block_add(gl_block, w, TRUE);

    /* effects */
    effects_block = gconf_block_new((GCallback)effects_changed, xc, 10);

    /* transparency */
    w = gconf_edit_boolean(effects_block, xconf_get(xc, "transparent"),
        _("Transparency"));
    gconf_block_add(effects_block, w, TRUE);

    color_block = gconf_block_new(NULL, NULL, INDENT_2);
    w = gtk_label_new(_("Color settings"));
    gconf_block_add(color_block, w, TRUE);
    w = gconf_edit_color(color_block, xconf_get(xc, "tintcolor"),
        xconf_get(xc, "alpha"));
    gconf_block_add(color_block, w, FALSE);

    gconf_block_add(effects_block, color_block->main, TRUE);

    /* round corners */
    w = gconf_edit_boolean(effects_block, xconf_get(xc, "roundcorners"),
        _("Round corners"));
    gconf_block_add(effects_block, w, TRUE);

    corner_block = gconf_block_new(NULL, NULL, INDENT_2);
    w = gtk_label_new(_("Radius is "));
    gconf_block_add(corner_block, w, TRUE);
    w = gconf_edit_int(geom_block, xconf_get(xc, "roundcornersradius"), 0, 30);
    gconf_block_add(corner_block, w, FALSE);
    w = gtk_label_new(_("pixels"));
    gconf_block_add(corner_block, w, FALSE);
    gconf_block_add(effects_block, corner_block->main, TRUE);

    /* auto hide */
    w = gconf_edit_boolean(effects_block, xconf_get(xc, "autohide"),
        _("Autohide"));
    gconf_block_add(effects_block, w, TRUE);

    ah_block = gconf_block_new(NULL, NULL, INDENT_2);
    w = gtk_label_new(_("Height when hidden is "));
    gconf_block_add(ah_block, w, TRUE);
    w = gconf_edit_int(ah_block, xconf_get(xc, "heightwhenhidden"), 0, 10);
    gconf_block_add(ah_block, w, FALSE);
    w = gtk_label_new(_("pixels"));
    gconf_block_add(ah_block, w, FALSE);
    gconf_block_add(effects_block, ah_block->main, TRUE);

    w = gconf_edit_int(effects_block, xconf_get(xc, "maxelemheight"), 0, 128);
    gconf_block_add(effects_block, gtk_label_new(_("Max Element Height")), TRUE);
    gconf_block_add(effects_block, w, FALSE);
    gconf_block_add(gl_block, effects_block->main, TRUE);

    /* empty row */
    gconf_block_add(gl_block, gtk_label_new(" "), TRUE);
}

/*********************************************************
 * panel properties
 *********************************************************/
static void
prop_changed(gconf_block *b)
{
    int i = 0;

    ENTER;
    XCG(b->data, "setlayer", &i, enum, bool_enum);
    gtk_widget_set_sensitive(layer_block->main, i);
    RET();
}

static void
mk_prop_block(xconf *xc)
{
    GtkWidget *w;

    ENTER;

    /* label */
    w = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(w), 0, 0.5);
    gtk_label_set_markup(GTK_LABEL(w), _("<b>Properties</b>"));
    gconf_block_add(gl_block, w, TRUE);

    /* properties */
    prop_block = gconf_block_new((GCallback)prop_changed, xc, 10);

    /* strut */
    w = gconf_edit_boolean(prop_block, xconf_get(xc, "setpartialstrut"),
        _("Do not cover by maximized windows"));
    gconf_block_add(prop_block, w, TRUE);

    w = gconf_edit_boolean(prop_block, xconf_get(xc, "setdocktype"),
        _("Set 'Dock' type"));
    gconf_block_add(prop_block, w, TRUE);

    /* set layer */
    w = gconf_edit_boolean(prop_block, xconf_get(xc, "setlayer"),
        _("Set stacking layer"));
    gconf_block_add(prop_block, w, TRUE);

    layer_block = gconf_block_new(NULL, NULL, INDENT_2);
    w = gtk_label_new(_("Panel is "));
    gconf_block_add(layer_block, w, TRUE);
    w = gconf_edit_enum(layer_block, xconf_get(xc, "layer"),
        layer_enum);
    gconf_block_add(layer_block, w, FALSE);
    w = gtk_label_new(_("all windows"));
    gconf_block_add(layer_block, w, FALSE);
    gconf_block_add(prop_block, layer_block->main, TRUE);


    gconf_block_add(gl_block, prop_block->main, TRUE);

    /* empty row */
    gconf_block_add(gl_block, gtk_label_new(" "), TRUE);
}

/*********************************************************
 * panel geometry
 *********************************************************/
static void
geom_changed(gconf_block *b)
{
    int i, j;

    ENTER;
    i = gtk_combo_box_get_active(GTK_COMBO_BOX(allign_opt));
    gtk_widget_set_sensitive(xmargin_spin, (i != ALLIGN_CENTER));
    i = gtk_combo_box_get_active(GTK_COMBO_BOX(width_opt));
    gtk_widget_set_sensitive(width_spin, (i != WIDTH_REQUEST));
    if (i == WIDTH_PERCENT)
        gtk_spin_button_set_range(GTK_SPIN_BUTTON(width_spin), 0, 100);
    else if (i == WIDTH_PIXEL) {
        XCG(b->data, "edge", &j, enum, edge_enum);
        gtk_spin_button_set_range(GTK_SPIN_BUTTON(width_spin), 0,
            (j == EDGE_RIGHT || j == EDGE_LEFT)
            ? gdk_screen_height() : gdk_screen_width());
    }
    RET();
}

static void
mk_geom_block(xconf *xc)
{
    GtkWidget *w;

    ENTER;

    /* label */
    w = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(w), 0, 0.5);
    gtk_label_set_markup(GTK_LABEL(w), _("<b>Geometry</b>"));
    gconf_block_add(gl_block, w, TRUE);

    /* geometry */
    geom_block = gconf_block_new((GCallback)geom_changed, xc, 10);

    w = gconf_edit_int(geom_block, xconf_get(xc, "width"), 0, 2300);
    gconf_block_add(geom_block, gtk_label_new(_("Width")), TRUE);
    gconf_block_add(geom_block, w, FALSE);
    width_spin = w;

    w = gconf_edit_enum(geom_block, xconf_get(xc, "widthtype"),
        widthtype_enum);
    gconf_block_add(geom_block, w, FALSE);
    width_opt = w;

    w = gconf_edit_int(geom_block, xconf_get(xc, "height"), 0, 300);
    gconf_block_add(geom_block, gtk_label_new(_("Height")), TRUE);
    gconf_block_add(geom_block, w, FALSE);

    w = gconf_edit_enum(geom_block, xconf_get(xc, "edge"),
        edge_enum);
    gconf_block_add(geom_block, gtk_label_new(_("Edge")), TRUE);
    gconf_block_add(geom_block, w, FALSE);

    w = gconf_edit_enum(geom_block, xconf_get(xc, "allign"),
        allign_enum);
    gconf_block_add(geom_block, gtk_label_new(_("Alignment")), TRUE);
    gconf_block_add(geom_block, w, FALSE);
    allign_opt = w;

    w = gconf_edit_int(geom_block, xconf_get(xc, "xmargin"), 0, 300);
    gconf_block_add(geom_block, gtk_label_new(_("X Margin")), TRUE);
    gconf_block_add(geom_block, w, FALSE);
    xmargin_spin = w;

    w = gconf_edit_int(geom_block, xconf_get(xc, "ymargin"), 0, 300);
    gconf_block_add(geom_block, gtk_label_new(_("Y Margin")), FALSE);
    gconf_block_add(geom_block, w, FALSE);
    ymargin_spin = w;

    gconf_block_add(gl_block, geom_block->main, TRUE);

    /* empty row */
    gconf_block_add(gl_block, gtk_label_new(" "), TRUE);
}

static GtkWidget *
mk_tab_global(xconf *xc)
{
    GtkWidget *page;

    ENTER;
    page = gtk_vbox_new(FALSE, 1);
    gtk_container_set_border_width(GTK_CONTAINER(page), 10);
    gl_block = gconf_block_new(NULL, NULL, 0);
    gtk_box_pack_start(GTK_BOX(page), gl_block->main, FALSE, TRUE, 0);

    mk_geom_block(xc);
    mk_prop_block(xc);
    mk_effects_block(xc);

    gtk_widget_show_all(page);

    geom_changed(geom_block);
    effects_changed(effects_block);
    prop_changed(prop_block);

    RET(page);
}

static GtkWidget *
mk_tab_profile(xconf *xc)
{
    GtkWidget *page, *label;
    gchar *s1;

    ENTER;
    page = gtk_vbox_new(FALSE, 1);
    gtk_container_set_border_width(GTK_CONTAINER(page), 10);

    s1 = g_strdup_printf(_("You're using '<b>%s</b>' profile, stored at\n"
            "<tt>%s</tt>"), panel_get_profile(), panel_get_profile_file());
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), s1);
    gtk_box_pack_start(GTK_BOX(page), label, FALSE, TRUE, 0);
    g_free(s1);

    gtk_widget_show_all(page);
    RET(page);
}

static void
dialog_response_event(GtkDialog *_dialog, gint rid, xconf *xc)
{
    xconf *oxc = g_object_get_data(G_OBJECT(dialog), "oxc");

    ENTER;
    if (rid == GTK_RESPONSE_APPLY ||
        rid == GTK_RESPONSE_OK)
    {
        DBG("apply changes\n");
        if (xconf_cmp(xc, oxc))
        {
            xconf_del(oxc, FALSE);
            oxc = xconf_dup(xc);
            g_object_set_data(G_OBJECT(dialog), "oxc", oxc);
            xconf_save_to_profile(xc);
            gtk_main_quit();
        }
    }
    if (rid == GTK_RESPONSE_DELETE_EVENT ||
        rid == GTK_RESPONSE_CLOSE ||
        rid == GTK_RESPONSE_OK)
    {
        gtk_widget_destroy(dialog);
        dialog = NULL;
        gconf_block_free(geom_block);
        gconf_block_free(gl_block);
        gconf_block_free(effects_block);
        gconf_block_free(color_block);
        gconf_block_free(corner_block);
        gconf_block_free(layer_block);
        gconf_block_free(prop_block);
        gconf_block_free(ah_block);
        xconf_del(xc, FALSE);
        xconf_del(oxc, FALSE);
    }
    RET();
}

static gboolean
dialog_cancel(GtkDialog *_dialog, GdkEvent *event, xconf *xc)
{
    dialog_response_event(_dialog, GTK_RESPONSE_CLOSE, xc);
    return FALSE;
}

static GtkWidget *
mk_dialog(xconf *oxc)
{
    GtkWidget *sw, *nb, *label;
    gchar *name;
    xconf *xc;

    ENTER;
    DBG("creating dialog\n");
    //name = g_strdup_printf("fbpanel settings: <%s> profile", cprofile);
    name = g_strdup_printf("fbpanel settings: <%s> profile",
        panel_get_profile());
    dialog = gtk_dialog_new_with_buttons (name,
        NULL,
        GTK_DIALOG_NO_SEPARATOR, //GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_APPLY,
        GTK_RESPONSE_APPLY,
        GTK_STOCK_OK,
        GTK_RESPONSE_OK,
        GTK_STOCK_CLOSE,
        GTK_RESPONSE_CLOSE,
        NULL);
    g_free(name);
    DBG("connecting sugnal to %p\n",  dialog);

    xc = xconf_dup(oxc);
    g_object_set_data(G_OBJECT(dialog), "oxc", xc);
    xc = xconf_dup(oxc);

    g_signal_connect (G_OBJECT(dialog), "response",
        (GCallback) dialog_response_event, xc);
    // g_signal_connect (G_OBJECT(dialog), "destroy",
    //  (GCallback) dialog_cancel, xc);
    g_signal_connect (G_OBJECT(dialog), "delete_event",
        (GCallback) dialog_cancel, xc);

    gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 500);
    gtk_window_set_icon_from_file(GTK_WINDOW(dialog),
        IMGPREFIX "/logo.png", NULL);

    nb = gtk_notebook_new();
    gtk_notebook_set_show_border (GTK_NOTEBOOK(nb), FALSE);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), nb);

    sw = mk_tab_global(xconf_get(xc, "global"));
    label = gtk_label_new(_("Panel"));
    gtk_misc_set_padding(GTK_MISC(label), 4, 1);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), sw, label);

    sw = mk_tab_plugins(xc);
    label = gtk_label_new(_("Plugins"));
    gtk_misc_set_padding(GTK_MISC(label), 4, 1);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), sw, label);

    sw = mk_tab_profile(xc);
    label = gtk_label_new(_("Profile"));
    gtk_misc_set_padding(GTK_MISC(label), 4, 1);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), sw, label);

    gtk_widget_show_all(dialog);
    RET(dialog);
}

void
configure(xconf *xc)
{
    ENTER;
    DBG("dialog %p\n",  dialog);
    if (!dialog)
        dialog = mk_dialog(xc);
    gtk_widget_show(dialog);
    RET();
}
