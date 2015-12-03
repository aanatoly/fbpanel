
/*
 *  test - test module. its purpose to continuously change its size by
 *  allocating and destroying widgets. It helps in debuging panels's
 *  geometry engine (panel.c )
 */
    


#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "panel.h"
#include "misc.h"
#include "plugin.h"

//#define DEBUGPRN
#include "dbg.h"


#define WID_NUM 80

typedef struct {
    plugin_instance plugin;
    GtkWidget *main;
    int count;
    int delta;
    int timer;
    GtkWidget *wid[WID_NUM];
} test_priv;

//static dclock me;

static gint
clock_update(gpointer data)
{
    test_priv *dc = (test_priv *)data;
    
    ENTER;
    if (dc->count >= WID_NUM-1)
        dc->delta = -1;
    else if (dc->count <= 0)
        dc->delta = 1;
    if (dc->delta == 1) {
        dc->wid[dc->count] = gtk_button_new_with_label("  wwwww  ");
        gtk_widget_show( dc->wid[dc->count] );
        gtk_box_pack_start(GTK_BOX(dc->main), dc->wid[dc->count], TRUE, FALSE, 0);
     } else
        gtk_widget_destroy(dc->wid[dc->count]);
    dc->count += dc->delta;
    RET(TRUE);
}


static int
test_constructor(plugin_instance *p)
{
    test_priv *dc;
    line s;
    
    ENTER;
    dc = (test_priv *) p;
    dc->delta = 1;
    while (get_line(p->fp, &s) != LINE_BLOCK_END) {
        ERR( "test: illegal in this context %s\n", s.str);
    }
    dc->main = p->panel->my_box_new(TRUE, 1);
    gtk_widget_show(dc->main);
    gtk_container_add(GTK_CONTAINER(p->pwid), dc->main);
    dc->timer = g_timeout_add(200, clock_update, (gpointer)dc);
    RET(1);
}


static void
test_destructor(plugin_instance *p)
{
    test_priv *dc = (test_priv *) p;

    ENTER;
    if (dc->timer)
        g_source_remove(dc->timer);
    gtk_widget_destroy(dc->main);
    RET();
}

static plugin_class class = {
    .count       = 0,
    .type        = "test",
    .name        = "Test plugin",
    .version     = "1.0",
    .description = "Creates and destroys widgets",
    .priv_size   = sizeof(test_priv),

    .constructor = test_constructor,
    .destructor  = test_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
