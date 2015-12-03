## miniconf makefiles ## 1.1 ##

TOPDIR := ..

TEXT = make_profile xlogout
all : $(TEXT)
CLEANLIST += make_profile

install :
	$Qinstall -d $(DESTDIR)$(LIBEXECDIR)
	$Qinstall -m 755 $(TEXT) $(DESTDIR)$(LIBEXECDIR)

include $(TOPDIR)/.config/rules.mk
