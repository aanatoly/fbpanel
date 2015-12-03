## miniconf makefiles ## 1.1 ##

TOPDIR := ../..

IMAGES = \
battery_0.png \
battery_1.png \
battery_2.png \
battery_3.png \
battery_4.png \
battery_5.png \
battery_6.png \
battery_7.png \
battery_8.png \
battery_charging_0.png \
battery_charging_1.png \
battery_charging_2.png \
battery_charging_3.png \
battery_charging_4.png \
battery_charging_5.png \
battery_charging_6.png \
battery_charging_7.png \
battery_charging_8.png \
battery_na.png \
dclock_glyphs.png            \
default.xpm                  \
gnome-session-halt.png       \
gnome-session-reboot.png     \
logo.png                     


install :
	$Qinstall -d $(DESTDIR)$(DATADIR)/images
	$Qinstall $(IMAGES) $(DESTDIR)$(DATADIR)/images

include $(TOPDIR)/.config/rules.mk
