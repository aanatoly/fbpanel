## miniconf makefiles ## 1.1 ##

TOPDIR := ..

exp_src = enums.c \
    fb-panel.c
exp_cflags = $(GTK2_CFLAGS) $(GMODULE2_CFLAGS) $(X11_CFLAGS)
exp_libs = $(GTK2_LIBS) $(GMODULE2_LIBS) $(X11_LIBS) -lm
exp_type = lib

include $(TOPDIR)/.config/rules.mk
