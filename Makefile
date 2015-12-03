## miniconf makefiles ## 1.1 ##

TOPDIR := .

SUBDIRS := data \
    exec \
    panel \
    plugins \
    po \
    scripts

include $(TOPDIR)/.config/rules.mk
