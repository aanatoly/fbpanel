/*
 * CPU usage plugin to fbpanel
 *
 * Copyright (C) 2004 by Alexandre Pereira da Silva <alexandre.pereira@poli.usp.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 */
/*A little bug fixed by Mykola <mykola@2ka.mipt.ru>:) */


#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <stdlib.h>

#include "plugin.h"
#include "panel.h"
#include "gtkbgbox.h"
#include "chart.h"


//#define DEBUGPRN
#include "dbg.h"


static void chart_add_tick(chart_priv *c, float *val);
static void chart_draw(chart_priv *c);
static void chart_size_allocate(GtkWidget *widget, GtkAllocation *a, chart_priv *c);
static gint chart_expose_event(GtkWidget *widget, GdkEventExpose *event, chart_priv *c);

static void chart_alloc_ticks(chart_priv *c);
static void chart_free_ticks(chart_priv *c);
static void chart_alloc_gcs(chart_priv *c, gchar *colors[]);
static void chart_free_gcs(chart_priv *c);

static void
chart_add_tick(chart_priv *c, float *val)
{
    int i;

    ENTER;
    if (!c->ticks)
        RET();
    for (i = 0; i < c->rows; i++) {
        if (val[i] < 0)
            val[i] = 0;
        if (val[i] > 1)        
            val[i] = 1;
        c->ticks[i][c->pos] = val[i] * c->h;
        DBG("new wval = %uld\n", c->ticks[i][c->pos]);
    }
    c->pos = (c->pos + 1) %  c->w;
    gtk_widget_queue_draw(c->da);

    RET();
}

static void
chart_draw(chart_priv *c)
{
    int j, i, y;

    ENTER;
    if (!c->ticks)
        RET();
    for (i = 1; i < c->w-1; i++) {
        y = c->h-2;
        for (j = 0; j < c->rows; j++) {
            int val;
	
            val = c->ticks[j][(i + c->pos) % c->w];
            if (val)
                gdk_draw_line(c->da->window, c->gc_cpu[j], i, y, i, y - val);
            y -= val;
        }
    }
    RET();
}

static void
chart_size_allocate(GtkWidget *widget, GtkAllocation *a, chart_priv *c)
{
    ENTER;
    if (c->w != a->width || c->h != a->height) {
        chart_free_ticks(c);
        c->w = a->width;
        c->h = a->height;
        chart_alloc_ticks(c);
        c->area.x = 0;
        c->area.y = 0;
        c->area.width = a->width;
        c->area.height = a->height;
        if (c->plugin.panel->transparent) {
            c->fx = 0;
            c->fy = 0;
            c->fw = a->width;
            c->fh = a->height;
        } else if (c->plugin.panel->orientation == GTK_ORIENTATION_HORIZONTAL) {
            c->fx = 0;
            c->fy = 1;
            c->fw = a->width;
            c->fh = a->height -2;
        } else {
            c->fx = 1;
            c->fy = 0;
            c->fw = a->width -2;
            c->fh = a->height;
        }
    }
    gtk_widget_queue_draw(c->da);
    RET();
}


static gint
chart_expose_event(GtkWidget *widget, GdkEventExpose *event, chart_priv *c)
{
    ENTER;
    gdk_window_clear(widget->window);
    chart_draw(c);

    gtk_paint_shadow(widget->style, widget->window,
        widget->state, GTK_SHADOW_ETCHED_IN,
        &c->area, widget, "frame", c->fx, c->fy, c->fw, c->fh);

#if 0
    gdk_draw_rectangle(widget->window, 
        widget->style->bg_gc[GTK_STATE_NORMAL],
        FALSE, 0, 0, c->w-1, c->h-1);
#endif    
    RET(FALSE);
}

static void
chart_alloc_ticks(chart_priv *c)
{
    int i;

    ENTER;
    if (!c->w || !c->rows)
        RET();
    c->ticks = g_new0(gint *, c->rows);
    for (i = 0; i < c->rows; i++) {
        c->ticks[i] = g_new0(gint, c->w);
        if (!c->ticks[i]) 
            DBG2("can't alloc mem: %p %d\n", c->ticks[i], c->w);
    }
    c->pos = 0;
    RET();
}


static void
chart_free_ticks(chart_priv *c)
{
    int i;

    ENTER;
    if (!c->ticks)
        RET();
    for (i = 0; i < c->rows; i++) 
        g_free(c->ticks[i]);
    g_free(c->ticks);
    c->ticks = NULL;
    RET();
}


static void
chart_alloc_gcs(chart_priv *c, gchar *colors[])
{
    int i;  
    GdkColor color;

    ENTER;
    c->gc_cpu = g_new0( typeof(*c->gc_cpu), c->rows);
    if (c->gc_cpu) {
        for (i = 0; i < c->rows; i++) {
            c->gc_cpu[i] = gdk_gc_new(c->plugin.panel->topgwin->window);
            gdk_color_parse(colors[i], &color);
            gdk_colormap_alloc_color(
                gdk_drawable_get_colormap(c->plugin.panel->topgwin->window),
                &color, FALSE, TRUE);
            gdk_gc_set_foreground(c->gc_cpu[i],  &color);
        }
    }
    RET();
}



static void
chart_free_gcs(chart_priv *c)
{
    int i;  

    ENTER;
    if (c->gc_cpu) {
        for (i = 0; i < c->rows; i++) 
            g_object_unref(c->gc_cpu[i]);            
        g_free(c->gc_cpu);
        c->gc_cpu = NULL;
    }
    RET();
}


static void
chart_set_rows(chart_priv *c, int num, gchar *colors[])
{    
    ENTER;
    g_assert(num > 0 && num < 10);
    chart_free_ticks(c);
    chart_free_gcs(c);
    c->rows = num;
    chart_alloc_ticks(c);
    chart_alloc_gcs(c, colors);
    RET();
}

static int
chart_constructor(plugin_instance *p)
{
    chart_priv *c;
    
    ENTER;
    /* must be allocated by caller */
    c = (chart_priv *) p;
    c->rows = 0;
    c->ticks = NULL;
    c->gc_cpu = NULL;
    c->da = p->pwid;

    gtk_widget_set_size_request(c->da, 40, 25);
    //gtk_container_set_border_width (GTK_CONTAINER (p->pwid), 1);
    g_signal_connect (G_OBJECT (p->pwid), "size-allocate",
          G_CALLBACK (chart_size_allocate), (gpointer) c);

    g_signal_connect_after (G_OBJECT (p->pwid), "expose-event",
          G_CALLBACK (chart_expose_event), (gpointer) c);
    
    RET(1);
}

static void
chart_destructor(plugin_instance *p)
{
    chart_priv *c = (chart_priv *) p;

    ENTER;
    chart_free_ticks(c);
    chart_free_gcs(c);
    RET();
}

static chart_class class = {
    .plugin = {
        .type        = "chart",
        .name        = "Chart",
        .description = "Basic chart plugin",
        .priv_size   = sizeof(chart_priv),

        .constructor = chart_constructor,
        .destructor  = chart_destructor,
    },
    .add_tick = chart_add_tick,
    .set_rows = chart_set_rows,
};
static plugin_class *class_ptr = (plugin_class *) &class;
