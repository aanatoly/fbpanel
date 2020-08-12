#ifndef MISC_H
#define MISC_H

#include <X11/Xatom.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <stdio.h>

#include "panel.h"

int str2num(xconf_enum *p, gchar *str, int defval);
gchar *num2str(xconf_enum *p, int num, gchar *defval);


extern void Xclimsg(Window win, long type, long l0, long l1, long l2, long l3, long l4);
void Xclimsgwm(Window win, Atom type, Atom arg);
extern void *get_xaproperty (Window win, Atom prop, Atom type, int *nitems);
char *get_textproperty(Window win, Atom prop);
void *get_utf8_property(Window win, Atom atom);
char **get_utf8_property_list(Window win, Atom atom, int *count);

void fb_init(void);
void fb_free(void);
//Window Select_Window(Display *dpy);
guint get_net_number_of_desktops();
extern guint get_net_current_desktop ();
extern guint get_net_wm_desktop(Window win);
extern void get_net_wm_state(Window win, net_wm_state *nws);
extern void get_net_wm_window_type(Window win, net_wm_window_type *nwwt);

void calculate_position(panel *np);
gchar *expand_tilda(gchar *file);

void get_button_spacing(GtkRequisition *req, GtkContainer *parent, gchar *name);
guint32 gcolor2rgb24(GdkColor *color);
gchar *gdk_color_to_RRGGBB(GdkColor *color);

GdkPixbuf *fb_pixbuf_new(gchar *iname, gchar *fname, int width, int height,
        gboolean use_fallback);
GtkWidget *fb_image_new(gchar *iname, gchar *fname, int width, int height);
GtkWidget *fb_button_new(gchar *iname, gchar *fname, int width, int height,
        gulong hicolor, gchar *name);


void menu_pos(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, GtkWidget *widget);

void configure();
gchar *indent(int level);

FILE *get_profile_file(gchar *profile, char *perm);

#endif
