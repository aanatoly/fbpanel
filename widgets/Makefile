## miniconf makefiles ## 1.1 ##

TOPDIR := ..

exp_src = \
	enums.c \
	fb-xed.c \
	fb-xed-int.c \
	fb-xed-enum.c \
	fb-xed-bool.c \
	fb-xed-color.c \
	fb-panel.c

exp_cflags = $(GTK2_CFLAGS) $(GMODULE2_CFLAGS) $(X11_CFLAGS)
exp_libs = $(GTK2_LIBS) $(GMODULE2_LIBS) $(X11_LIBS) -lm
exp_type = lib

test_src = test.c
test_cflags = $(GTK2_CFLAGS) $(GMODULE2_CFLAGS) $(X11_CFLAGS)
test_libs = $(GTK2_LIBS) $(GMODULE2_LIBS) $(X11_LIBS) -lm
test_type = bin
test:libexp.so

include $(TOPDIR)/.config/rules.mk
