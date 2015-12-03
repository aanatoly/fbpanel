// reused dclock.c and variables from pager.c
// 11/23/04 by cmeury@users.sf.net",

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"

//#define DEBUGPRN
#include "dbg.h"

typedef struct {
    plugin_instance plugin;
    GtkWidget *main;
    GtkWidget *namew;
    int deskno, desknum;
} deskno_priv;

static void
change_desktop(deskno_priv *dc, int delta)
{
    int newdesk = dc->deskno + delta;

    ENTER;
    if (newdesk < 0)
        newdesk = dc->desknum - 1;
    else if (newdesk >= dc->desknum)
        newdesk = 0;
    DBG("%d/%d -> %d\n", dc->deskno, dc->desknum, newdesk);
    Xclimsg(GDK_ROOT_WINDOW(), a_NET_CURRENT_DESKTOP, newdesk, 0, 0, 0, 0);
    RET();
}

static void
clicked(GtkWidget *widget, deskno_priv *dc)
{
    ENTER;
    change_desktop(dc, 1);
}
 
static gboolean
scrolled(GtkWidget *widget, GdkEventScroll *event, deskno_priv *dc)
{
    ENTER;
    change_desktop(dc, (event->direction == GDK_SCROLL_UP
            || event->direction == GDK_SCROLL_LEFT) ? -1 : 1);
    return FALSE;
}

static gint
name_update(GtkWidget *widget, deskno_priv *dc)
{
    char buffer [15];

    ENTER;
    dc->deskno = get_net_current_desktop();
    sprintf(buffer, "<b>%d</b>", dc->deskno + 1);
    gtk_label_set_markup(GTK_LABEL(dc->namew), buffer);
    RET(TRUE);
}

static gint
update(GtkWidget *widget, deskno_priv *dc)
{
    ENTER;
    dc->desknum = get_net_number_of_desktops();
    RET(TRUE);
}

static int
deskno_constructor(plugin_instance *p)
{
    deskno_priv *dc;
    
    ENTER;
    dc = (deskno_priv *) p;
    dc->main = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(dc->main), GTK_RELIEF_NONE);
    g_signal_connect(G_OBJECT(dc->main), "clicked", G_CALLBACK(clicked),
        (gpointer) dc);
    g_signal_connect(G_OBJECT(dc->main), "scroll-event", G_CALLBACK(scrolled),
        (gpointer) dc);
    dc->namew = gtk_label_new("ww");
    gtk_container_add(GTK_CONTAINER(dc->main), dc->namew);
    gtk_container_add(GTK_CONTAINER(p->pwid), dc->main);
    //gtk_widget_add_events(p->pwid, GDK_SCROLL_MASK);
    //gtk_widget_add_events(dc->main, GDK_SCROLL_MASK);
    gtk_widget_show_all(p->pwid);
    name_update(dc->main, dc);
    update(dc->main, dc);
    g_signal_connect(G_OBJECT(fbev), "current_desktop", G_CALLBACK
        (name_update), (gpointer) dc);
    g_signal_connect(G_OBJECT(fbev), "number_of_desktops", G_CALLBACK
        (update), (gpointer) dc);
    RET(1);
}


static void
deskno_destructor(plugin_instance *p)
{
  deskno_priv *dc = (deskno_priv *) p;

  ENTER;
  g_signal_handlers_disconnect_by_func(G_OBJECT(fbev), name_update, dc);
  g_signal_handlers_disconnect_by_func(G_OBJECT(fbev), update, dc); 
  RET();
}

static plugin_class class = {
    .type        = "deskno",
    .name        = "Desktop No v1",
    .version     = "0.6",
    .description = "Display workspace number",
    .priv_size   = sizeof(deskno_priv),

    .constructor = deskno_constructor,
    .destructor  = deskno_destructor,
};
static plugin_class *class_ptr = (plugin_class *) &class;
