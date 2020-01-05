## miniconf makefiles ## 1.1 ##

TOPDIR := ../..

runbox_src = runbox.c
runbox_cflags = -DPLUGIN $(GTK2_CFLAGS) $(shell pkg-config --cflags dbus-glib-1)
runbox_libs = $(GTK2_LIBS) $(shell pkg-config --libs dbus-glib-1)
runbox_type = lib 

include $(TOPDIR)/.config/rules.mk
