## miniconf makefiles ## 1.1 ##

TOPDIR := ../..

TEXT = fbpanel.1
all : $(TEXT)
CLEANLIST += $(TEXT)

install :
	$Qinstall -d $(DESTDIR)$(MANDIR)
	$Qinstall -m 644 $(TEXT) $(DESTDIR)$(MANDIR)

include $(TOPDIR)/.config/rules.mk
