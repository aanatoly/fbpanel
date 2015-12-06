#include <stdio.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include "fb-panel.h"
#include "fb-xed.h"
#include "fb-xed-int.h"
#include "fb-xed-enum.h"
#include "fb-xed-bool.h"
#include "fb-xed-color.h"

#define DEBUGPRN
#include "dbg.h"

static void
scan_props_enum(gpointer data)
{
    GParamSpecEnum *ps = data;
    GEnumClass *ec = ps->enum_class;
    GEnumValue *ev;
    int i;

    ev = g_enum_get_value(ec, ps->default_value);
    DBGE("    default '%s' from [", ev->value_nick);
    for (i = 0; i < ec->n_values; i++)
        DBGE(" '%s'", ec->values[i].value_nick);
    DBGE("]\n");
}

static void
scan_props_int(gpointer data)
{
    GParamSpecInt *ps = data;
    DBGE("    default %d, range %d - %d\n",
        ps->default_value, ps->minimum, ps->maximum);
}


static void
scan_props_string(gpointer data)
{
    GParamSpecString *ps = data;
    DBGE("    default '%s', first '%s'\n",
        ps->default_value, ps->cset_first);
}


static gboolean
scan_props(gpointer data)
{
    FbPanel *p = data;
    GType gt;
    GParamSpec **ps;
    guint nps;
    GType pgt;
    GParamSpec **pps;
    guint pnps;
    gint i;


    gt = G_OBJECT_TYPE(p);
    DBGE("object '%s'\n", g_type_name(gt));
    pgt = g_type_parent(gt);
    DBGE("parent '%s'\n", g_type_name(pgt));

    ps = g_object_class_list_properties(
        G_OBJECT_CLASS(g_type_class_ref(gt)), &nps);
    DBGE("obj props: %d\n", nps);
    pps = g_object_class_list_properties(
        G_OBJECT_CLASS(g_type_class_ref(pgt)), &pnps);
    DBGE("parent props: %d\n", pnps);

    for (i = pnps; i < nps; i++) {
        DBGE(" %i: name '%s', %s, nick '%s'\n", i,
            g_param_spec_get_name(ps[i]),
            g_type_name(G_OBJECT_TYPE(ps[i])),
            g_param_spec_get_nick(ps[i]));
        if (G_IS_PARAM_SPEC_STRING(ps[i]))
            scan_props_string(ps[i]);
        else if (G_IS_PARAM_SPEC_INT(ps[i]))
            scan_props_int(ps[i]);
        else if (G_IS_PARAM_SPEC_ENUM(ps[i]))
            scan_props_enum(ps[i]);
        else
            DBGE("    ignore\n");
    }
    g_object_set(data, "width-type", FB_WIDTH_DYNAMIC, NULL);
    g_object_set(data, "width", i, NULL);
    g_object_set(data, "height", i, NULL);
    g_free(pps);
    g_free(ps);
    return FALSE;
}

static GtkWidget *
conf_gui_new(GObject *obj)
{
    GtkWidget *w, *vbox, *ex;
    GtkSizeGroup *sg;

    sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(w), 100, 100);
    gtk_container_set_border_width(GTK_CONTAINER(w), 10);
    vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_add(GTK_CONTAINER(w), vbox);
    ex = fb_xed_enum_new(obj, "width-type", sg);
    gtk_box_pack_start(GTK_BOX(vbox), ex, FALSE, TRUE, 0);
    ex = fb_xed_int_new(obj, "width", sg);
    gtk_box_pack_start(GTK_BOX(vbox), ex, FALSE, TRUE, 0);
    ex = fb_xed_int_new(obj, "height", sg);
    gtk_box_pack_start(GTK_BOX(vbox), ex, FALSE, TRUE, 0);
    ex = fb_xed_enum_new(obj, "edge", sg);
    gtk_box_pack_start(GTK_BOX(vbox), ex, FALSE, TRUE, 0);
    ex = fb_xed_bool_new(obj, "transparent");
    gtk_box_pack_start(GTK_BOX(vbox), ex, FALSE, TRUE, 0);
    ex = fb_xed_color_new(obj, "tint", sg);
    gtk_box_pack_start(GTK_BOX(vbox), ex, FALSE, TRUE, 0);
    ex = fb_xed_int_new(obj, "pos-x", sg);
    gtk_box_pack_start(GTK_BOX(vbox), ex, FALSE, TRUE, 0);
    ex = fb_xed_int_new(obj, "pos-y", sg);
    gtk_box_pack_start(GTK_BOX(vbox), ex, FALSE, TRUE, 0);
    gtk_widget_show_all(w);
    return w;
}



int
main(int argc, char *argv[])
{
    GtkWidget *p, *w;

    gtk_init (&argc, &argv);
    p = fb_panel_new("test");
    gtk_widget_show_all(p);
    w = conf_gui_new(G_OBJECT(p));
    gtk_widget_show(w);
    /* w = conf_gui_new(G_OBJECT(p)); */
    /* gtk_widget_show(w); */
    g_timeout_add(3000, scan_props, p);
    gtk_main();
    return 0;
}
