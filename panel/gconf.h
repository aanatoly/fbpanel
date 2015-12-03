
#ifndef _GCONF_H_
#define _GCONF_H_

#include <gtk/gtk.h>
#include "panel.h"

typedef struct
{
    GtkWidget *main, *area;
    GCallback cb;
    gpointer data;
    GSList *rows;
    GtkSizeGroup *sgr;
} gconf_block;


gconf_block *gconf_block_new(GCallback cb, gpointer data, int indent);
void gconf_block_free(gconf_block *b);
void gconf_block_add(gconf_block *b, GtkWidget *w, gboolean new_row);

GtkWidget *gconf_edit_int(gconf_block *b, xconf *xc, int min, int max);
GtkWidget *gconf_edit_enum(gconf_block *b, xconf *xc, xconf_enum *e);
GtkWidget *gconf_edit_boolean(gconf_block *b, xconf *xc, gchar *text);
GtkWidget *gconf_edit_color(gconf_block *b, xconf *xc_color, xconf *xc_alpha);

#endif
