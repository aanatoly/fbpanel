
#include "plugin.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <gdk/gdk.h>
#include <string.h>
#include <stdlib.h>



#include "misc.h"
#include "bg.h"
#include "gtkbgbox.h"


//#define DEBUGPRN
#include "dbg.h"
extern panel *the_panel;


/**************************************************************/
static GHashTable *class_ht;


void
class_register(plugin_class *p)
{
    ENTER;
    if (!class_ht) {
        class_ht = g_hash_table_new(g_str_hash, g_str_equal);
        DBG("creating class hash table\n");
    }
    DBG("registering %s\n", p->type);
    if (g_hash_table_lookup(class_ht, p->type)) {
        ERR("Can't register plugin %s. Such name already exists.\n", p->type);
        exit(1);
    }
    p->dynamic = (the_panel != NULL); /* dynamic modules register after main */
    g_hash_table_insert(class_ht, p->type, p);
    RET();
}

void
class_unregister(plugin_class *p)
{
    ENTER;
    DBG("unregistering %s\n", p->type);
    if (!g_hash_table_remove(class_ht, p->type)) {
        ERR("Can't unregister plugin %s. No such name\n", p->type);
    }
    if (!g_hash_table_size(class_ht)) {
        DBG("dwstroying class hash table\n");
        g_hash_table_destroy(class_ht);
        class_ht = NULL;
    }
    RET();
}

void
class_put(char *name)
{
    GModule *m;
    gchar *s;
    plugin_class *tmp;

    ENTER;
    DBG("%s\n", name);
    if (!(class_ht && (tmp = g_hash_table_lookup(class_ht, name))))
        RET();
    tmp->count--;
    if (tmp->count || !tmp->dynamic)
        RET();

    s = g_strdup_printf(LIBDIR "/lib%s.so", name);
    DBG("loading module %s\n", s);
    m = g_module_open(s, G_MODULE_BIND_LAZY);
    g_free(s);
    if (m) {
        /* Close it twise to undo initial open in class_get */
        g_module_close(m);
        g_module_close(m);
    }
    RET();
}

gpointer
class_get(char *name)
{
    GModule *m;
    plugin_class *tmp;
    gchar *s;

    ENTER;
    DBG("%s\n", name);
    if (class_ht && (tmp = g_hash_table_lookup(class_ht, name))) {
        DBG("found\n");
        tmp->count++;
        RET(tmp);
    }
    s = g_strdup_printf(LIBDIR "/lib%s.so", name);
    DBG("loading module %s\n", s);
    m = g_module_open(s, G_MODULE_BIND_LAZY);
    g_free(s);
    if (m) {
        if (class_ht && (tmp = g_hash_table_lookup(class_ht, name))) {
            DBG("found\n");
            tmp->count++;
            RET(tmp);
        }
    }
    ERR("%s\n", g_module_error());
    RET(NULL);
}



/**************************************************************/

plugin_instance *
plugin_load(char *type)
{
    plugin_class *pc = NULL;
    plugin_instance  *pp = NULL;

    ENTER;
    /* nothing was found */
    if (!(pc = class_get(type)))
        RET(NULL);

    DBG("%s priv_size=%d\n", pc->type, pc->priv_size);
    pp = g_malloc0(pc->priv_size);
    g_return_val_if_fail (pp != NULL, NULL);
    pp->class = pc;
    RET(pp);
}


void
plugin_put(plugin_instance *this)
{
    gchar *type;

    ENTER;
    type = this->class->type;
    g_free(this);
    class_put(type);
    RET();
}

gboolean panel_button_press_event(GtkWidget *widget, GdkEventButton *event,
        panel *p);

int
plugin_start(plugin_instance *this)
{
    ENTER;

    DBG("%s\n", this->class->type);
    if (!this->class->invisible) {
        this->pwid = gtk_bgbox_new();
        gtk_widget_set_name(this->pwid, this->class->type);
        gtk_box_pack_start(GTK_BOX(this->panel->box), this->pwid, this->expand,
                TRUE, this->padding);
        DBG("%s expand %d\n", this->class->type, this->expand);
        gtk_container_set_border_width(GTK_CONTAINER(this->pwid), this->border);
        DBG("here this->panel->transparent = %d\n", this->panel->transparent);
        if (this->panel->transparent) {
            DBG("here g\n");
            gtk_bgbox_set_background(this->pwid, BG_INHERIT,
                    this->panel->tintcolor, this->panel->alpha);
        }
        DBG("here\n");
        g_signal_connect (G_OBJECT (this->pwid), "button-press-event",
              (GCallback) panel_button_press_event, this->panel);
        gtk_widget_show(this->pwid);
        DBG("here\n");
    } else {
        /* create a no-window widget and do not show it it's usefull to have
         * unmaped widget for invisible plugins so their indexes in plugin list
         * are the same as in panel->box. required for children reordering */
        this->pwid = gtk_vbox_new(TRUE, 0);
        gtk_box_pack_start(GTK_BOX(this->panel->box), this->pwid, FALSE,
                TRUE,0);
        gtk_widget_hide(this->pwid);
    }
    DBG("here\n");
    if (!this->class->constructor(this)) {
        DBG("here\n");
        gtk_widget_destroy(this->pwid);
        RET(0);
    }
    RET(1);
}


void
plugin_stop(plugin_instance *this)
{
    ENTER;
    DBG("%s\n", this->class->type);
    this->class->destructor(this);
    this->panel->plug_num--;
    gtk_widget_destroy(this->pwid);
    RET();
}


GtkWidget *
default_plugin_edit_config(plugin_instance *pl)
{
    GtkWidget *vbox, *label;
    gchar *msg;

    ENTER;
    vbox = gtk_vbox_new(FALSE, 0);
    /* XXX: harcoded default profile name */
    msg = g_strdup_printf("Graphical '%s' plugin configuration\n is not "
          "implemented yet.\n"
          "Please edit manually\n\t~/.config/fbpanel/default\n\n"
          "You can use as example files in \n\t%s/share/fbpanel/\n"
          "or visit\n"
          "\thttp://fbpanel.sourceforge.net/docs.html", pl->class->name,
          PREFIX);
    label = gtk_label_new(msg);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_box_pack_end(GTK_BOX(vbox), label, TRUE, TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 14);
    g_free(msg);

    RET(vbox);
}
