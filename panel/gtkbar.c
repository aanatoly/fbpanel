/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "gtkbar.h"

//#define DEBUGPRN
#include "dbg.h"

#define MAX_CHILD_SIZE 150

static void gtk_bar_class_init    (GtkBarClass   *klass);
static void gtk_bar_size_request  (GtkWidget *widget, GtkRequisition *requisition);
static void gtk_bar_size_allocate (GtkWidget *widget, GtkAllocation  *allocation);
//static gint gtk_bar_expose        (GtkWidget *widget, GdkEventExpose *event);
float ceilf(float x);

static GtkBoxClass *parent_class = NULL;

GType
gtk_bar_get_type (void)
{
    static GType bar_type = 0;

    if (!bar_type)
    {
        static const GTypeInfo bar_info =
            {
                sizeof (GtkBarClass),
                NULL,		/* base_init */
                NULL,		/* base_finalize */
                (GClassInitFunc) gtk_bar_class_init,
                NULL,		/* class_finalize */
                NULL,		/* class_data */
                sizeof (GtkBar),
                0,		/* n_preallocs */
                NULL
            };

        bar_type = g_type_register_static (GTK_TYPE_BOX, "GtkBar",
              &bar_info, 0);
    }

    return bar_type;
}

static void
gtk_bar_class_init (GtkBarClass *class)
{
    GtkWidgetClass *widget_class;

    parent_class = g_type_class_peek_parent (class);
    widget_class = (GtkWidgetClass*) class;

    widget_class->size_request = gtk_bar_size_request;
    widget_class->size_allocate = gtk_bar_size_allocate;
    //widget_class->expose_event = gtk_bar_expose;

}


GtkWidget*
gtk_bar_new(GtkOrientation orient, gint spacing,
    gint child_height, gint child_width)
{
    GtkBar *bar;

    bar = g_object_new (GTK_TYPE_BAR, NULL);
    GTK_BOX (bar)->spacing = spacing;
    bar->orient = orient;
    bar->child_width = MAX(1, child_width);
    bar->child_height = MAX(1, child_height);
    bar->dimension = 1;
    return (GtkWidget *)bar;
}

void
gtk_bar_set_dimension(GtkBar *bar, gint dimension)
{
    dimension = MAX(1, dimension);
    if (bar->dimension != dimension) {
        bar->dimension = MAX(1, dimension);
        gtk_widget_queue_resize(GTK_WIDGET(bar));
    }
}

gint gtk_bar_get_dimension(GtkBar *bar)
{
    return bar->dimension;
}

static void
gtk_bar_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
    GtkBox *box = GTK_BOX(widget);
    GtkBar *bar = GTK_BAR(widget);;
    GtkBoxChild *child;
    GList *children;
    gint nvis_children, rows, cols, dim;
    
    nvis_children = 0;
    children = box->children;
    while (children) {
        child = children->data;
        children = children->next;

        if (GTK_WIDGET_VISIBLE(child->widget))	{
            GtkRequisition child_requisition;
            
            /* Do not remove child request !!! Label's proper layout depends
             * on request running before alloc. */ 
            gtk_widget_size_request(child->widget, &child_requisition);
            nvis_children++;
        }
    }
    DBG("nvis_children=%d\n", nvis_children);
    if (!nvis_children) {
        requisition->width = 2;
        requisition->height = 2;
        return;
    }
    dim = MIN(bar->dimension, nvis_children);
    if (bar->orient == GTK_ORIENTATION_HORIZONTAL) {
        rows = dim;
        cols = (gint) ceilf((float) nvis_children / rows);
    } else {
        cols = dim;
        rows = (gint) ceilf((float) nvis_children / cols);
    }
    
    requisition->width = bar->child_width * cols
        + box->spacing * (cols - 1);
    requisition->height = bar->child_height * rows
        + box->spacing * (rows - 1);
    DBG("width=%d, height=%d\n", requisition->width, requisition->height);
}

static void
gtk_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    GtkBox *box;
    GtkBar *bar;
    GtkBoxChild *child;
    GList *children;
    GtkAllocation child_allocation;
    gint nvis_children, tmp, rows, cols, dim;

    ENTER;
    DBG("a.w=%d  a.h=%d\n", allocation->width, allocation->height);
    box = GTK_BOX (widget);
    bar = GTK_BAR (widget);
    widget->allocation = *allocation;

    nvis_children = 0;
    children = box->children;
    while (children) {
        child = children->data;
        children = children->next;
        
        if (GTK_WIDGET_VISIBLE (child->widget))
            nvis_children += 1;
    }
    gtk_widget_queue_draw(widget);
    dim = MIN(bar->dimension, nvis_children);
    if (nvis_children == 0)
        RET();
    if (bar->orient == GTK_ORIENTATION_HORIZONTAL) {
        rows = dim;
        cols = (gint) ceilf((float) nvis_children / rows);
    } else {
        cols = dim;
        rows = (gint) ceilf((float) nvis_children / cols);
    }
    DBG("rows=%d cols=%d\n", rows, cols);
    tmp = allocation->width - (cols - 1) * box->spacing;
    child_allocation.width = MIN(tmp / cols, bar->child_width);
    tmp = allocation->height - (rows - 1) * box->spacing;
    child_allocation.height = MIN(tmp / rows, bar->child_height);

    if (child_allocation.width < 1)
        child_allocation.width = 1;
    if (child_allocation.height < 1)
        child_allocation.height = 1;
    DBG("child alloc: width=%d height=%d\n",
        child_allocation.width,
        child_allocation.height);
        
    child_allocation.x = allocation->x;
    child_allocation.y = allocation->y;
    children = box->children;
    tmp = 0;
    while (children) {
        child = children->data;
        children = children->next;
        
        if (GTK_WIDGET_VISIBLE (child->widget)) {
            DBG("allocate x=%d y=%d\n", child_allocation.x,
                child_allocation.y);
            gtk_widget_size_allocate(child->widget, &child_allocation);
            tmp++;
            if (tmp == cols) {
                child_allocation.x = allocation->x;
                child_allocation.y += child_allocation.height + box->spacing;
                tmp = 0;
            } else {
                child_allocation.x += child_allocation.width + box->spacing;
            }
        }
    }
    RET();
}


#if 0
static gint
gtk_bar_expose (GtkWidget *widget, GdkEventExpose *event)
{
    ENTER;

    if (GTK_WIDGET_DRAWABLE (widget)) {
        int w, h;
        
        DBG("w, h = %d,%d\n", w, h);
        if (!GTK_WIDGET_APP_PAINTABLE (widget))
            gtk_paint_flat_box (widget->style, widget->window,
                  widget->state, GTK_SHADOW_NONE,
                  NULL /*&event->area*/, widget, NULL,
                  0, 0, w, h);
    }
    RET(FALSE);
}
#endif
