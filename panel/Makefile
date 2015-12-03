## miniconf makefiles ## 1.1 ##

TOPDIR := ..

fbpanel_src = bg.c \
    ev.c \
    gconf.c \
    gconf_panel.c \
    gconf_plugins.c \
    gtkbar.c \
    gtkbgbox.c \
    misc.c \
    panel.c \
    plugin.c \
    run.c \
    xconf.c
fbpanel_cflags = $(GTK2_CFLAGS) $(GMODULE2_CFLAGS) $(X11_CFLAGS) 
fbpanel_libs = $(GTK2_LIBS) $(GMODULE2_LIBS) $(X11_LIBS) -lm
fbpanel_type = bin 

include $(TOPDIR)/.config/rules.mk
