

#include "panel.h"
#include "misc.h"
#include "plugin.h"


//#define DEBUGPRN
#include "dbg.h"


static int
separator_constructor(plugin_instance *p)
{
    GtkWidget *sep;
      
    ENTER;
    sep = p->panel->my_separator_new();
    gtk_container_add(GTK_CONTAINER(p->pwid), sep);
    gtk_widget_show_all(p->pwid);
    RET(1);
}

static void
separator_destructor(plugin_instance *p)
{
    ENTER; 
    RET();
}


static plugin_class class = {
    .count = 0,
    .type        = "separator",
    .name        = "Separator",
    .version     = "1.0",
    .description = "Separator line",
    .priv_size   = sizeof(plugin_instance),

    .constructor = separator_constructor,
    .destructor  = separator_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
