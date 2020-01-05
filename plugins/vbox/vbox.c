#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "gtkbgbox.h"

//#define DEBUGPRN
#include "dbg.h"


typedef struct {
    plugin_instance plugin;
    panel *panel2;

} vbox_priv;

static void
vbox_destructor(plugin_instance *p)
{
    vbox_priv *vb;

    ENTER;

    vb = (vbox_priv *) p;
    free(vb->panel2);

    RET();
}


// based on panel.c BEGIN //
static void
panel_parse_plugin(xconf *xc, panel *p)
{
    plugin_instance *plug = NULL;
    gchar *type = NULL;

    ENTER;
    xconf_get_str(xconf_find(xc, "type", 0), &type);
    if (!type || !(plug = plugin_load(type))) {
        ERR( "fbpanel: can't load %s plugin\n", type);
        return;
    }
    plug->panel = p;
    XCG(xc, "expand", &plug->expand, enum, bool_enum);
    XCG(xc, "padding", &plug->padding, int);
    XCG(xc, "border", &plug->border, int);
    plug->xc = xconf_find(xc, "config", 0);

    if (!plugin_start(plug)) {
        ERR( "fbpanel: can't start plugin %s\n", type);
        exit(1);
    }
    p->plugins = g_list_append(p->plugins, plug);
}
// based on panel.c END //


static int
vbox_constructor(plugin_instance *p)
{
    vbox_priv *vb;

    ENTER;
    vb = (vbox_priv *) p;

    vb->panel2 = malloc(sizeof(panel));
    memcpy(vb->panel2, p->panel, sizeof(panel));


    if (p->panel->orientation == GTK_ORIENTATION_HORIZONTAL) {
        vb->panel2->box = gtk_vbox_new(TRUE, 1);
    } else {
        vb->panel2->box = gtk_hbox_new(TRUE, 1);
    }
    gtk_container_set_border_width (GTK_CONTAINER (vb->panel2->box), 0);
    gtk_box_set_homogeneous(GTK_BOX(vb->panel2->box), FALSE);
    gtk_widget_show(vb->panel2->box);

    //gtk_bgbox_set_background(p->pwid, BG_STYLE, 0, 0);
    gtk_container_set_border_width (GTK_CONTAINER (p->pwid), 0);
    gtk_container_add(GTK_CONTAINER(p->pwid), vb->panel2->box);


    xconf *nxc;
    GSList *pl;
    for (pl = p->xc->sons; pl ; pl = g_slist_next(pl))
    {
        nxc = pl->data;
        if (!strcmp(nxc->name, "Plugin"))
            panel_parse_plugin(nxc, vb->panel2);
    }

    RET(1);
}

static plugin_class class = {
    .fname       = NULL,
    .count       = 0,
    .type        = "vbox",
    .name        = "Vbox",
    .version     = "1.0",
    .description = "Testowy plugin rrp",
    .priv_size   = sizeof(vbox_priv),

    .constructor = vbox_constructor,
    .destructor  = vbox_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
