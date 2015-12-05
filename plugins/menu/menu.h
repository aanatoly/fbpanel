#ifndef MENU_H
#define MENU_H


#include "plugin.h"
#include "panel.h"

#define MENU_DEFAULT_ICON_SIZE 22

typedef struct {
    plugin_instance plugin;
    GtkWidget *menu, *bg;
    int iconsize, paneliconsize;
    xconf *xc;
    guint tout, rtout;
    gboolean has_system_menu;
    time_t btime;
    gint icon_size;
} menu_priv;

typedef struct {
    plugin_class plugin;
} menu_class;


#endif
