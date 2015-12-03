#ifndef CHART_H
#define CHART_H


#include "plugin.h"
#include "panel.h"


/* chart.h */
typedef struct {
    plugin_instance plugin;
    GdkGC **gc_cpu;
    GtkWidget *da;

    gint **ticks;
    gint pos;
    gint w, h, rows;
    GdkRectangle area; /* frame area and exact positions */
    int fx, fy, fw, fh; 
} chart_priv;

typedef struct {
    plugin_class plugin;
    void (*add_tick)(chart_priv *c, float *val);
    void (*set_rows)(chart_priv *c, int num, gchar *colors[]);
} chart_class;


#endif
