## miniconf makefiles ## 1.1 ##

TOPDIR := ../..

TEXT = default pager
all : $(TEXT)
CLEANLIST += $(TEXT)

install :
	$Qinstall -d $(DESTDIR)$(DATADIR)
	$Qinstall -m 644 $(TEXT) $(DESTDIR)$(DATADIR)

include $(TOPDIR)/.config/rules.mk
