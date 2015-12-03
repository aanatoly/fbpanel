#ifndef meter_H
#define meter_H

#include "plugin.h"
#include "panel.h"

typedef struct {
    plugin_instance plugin;
    gchar **icons;
    gint num;
    GtkWidget *meter;
    gfloat level;
    gint cur_icon;
    gint size;
    gint itc_id;
} meter_priv;

typedef struct {
    plugin_class plugin;
    void (*set_level)(meter_priv *c, int val);
    void (*set_icons)(meter_priv *c, gchar **icons);
} meter_class;


#endif
