
###############################################
#  environment checks                         #
###############################################

SHELL=/bin/bash

.DEFAULT_GOAL := all

define prnvar
$(warning $(origin $1) $1='$($1)')
endef

$(TOPDIR)/config.mk: $(TOPDIR)/version $(TOPDIR)/.config/*
	@echo Please run $(TOPDIR)/configure
	@echo
	@false
include $(TOPDIR)/config.mk


ifeq ($(realpath $(TOPDIR)),$(CURDIR))
IS_TOPDIR := yes
else
IS_TOPDIR := no
endif

IS_ALL := $(filter all,$(if $(MAKECMDGOALS),$(MAKECMDGOALS),$(.DEFAULT_GOAL)))
IS_HELP := $(filter help,$(MAKECMDGOALS))

ifeq ($(NV),)
export NV = $(PROJECT_NAME)-$(PROJECT_VERSION)
endif

EMPTY :=
SPACE := $(EMPTY) $(EMPTY)

###############################################
#  recurion rules                             #
###############################################

RGOALS = all clean install svnignore
.PHONY : $(RGOALS) $(SUBDIRS)
$(RGOALS) : $(SUBDIRS)

$(SUBDIRS):
	$Q$(MAKE) -S -C $@ $(MAKECMDGOALS)
unexport SUBDIRS


$(RGOALS) $(SUBDIRS) : FORCE
FORCE:
	@#


###############################################
#  help                                       #
###############################################

# Help targets allow to shed a light on what your makefile does.
# Text for targets and variables will be formated into nice columns,
# while help for examples will be printed as is.
#
# You can have any number of these, if you use '::' synatx
#
# The syntax is
# help_target ::
# 	echo "name - explanation, every"
# 	echo "    next line is indented"
#
# help_variable ::
# 	echo "name - explanation, every"
# 	echo "    next line is indented"
#
# help_example ::
# 	echo "@@"
# 	echo "Any multi-line text formated as desired"

help :
	$Q$(MAKE) -j1 V=0 help_target | $(TOPDIR)/.config/help target
	$Q$(MAKE) -j1 V=0 help_variable | $(TOPDIR)/.config/help variable
ifeq ($V,1)
	$Q$(MAKE) -j1 V=0 help_example | $(TOPDIR)/.config/help example
endif

help_target ::
	@echo "help    - print this help"


###############################################
#  output customization                       #
###############################################

help_variable ::
	@echo "V - verbose output, if non-null"

help_example ::
	@echo "@@"
	@echo "Verbose output"
	@echo "  make V=1"
	@echo "  make V=1 clean"
	@echo "  make V=1 tar"

ifeq ($(MAKELEVEL),0)
export STARTDIR:=$(CURDIR)
endif

# make V=1 - very verbose, prints all commands
# make V=0 - prints only titles [default]
ifeq ($V$(IS_HELP),1)
override Q :=
else
override Q := @
MAKEFLAGS += --no-print-directory
out := 2>/dev/null 1>/dev/null
endif
summary = @echo " $(1)" $(subst $(STARTDIR)/,,$(CURDIR)/)$(2)
summary2 = @printf " %-5s %s\n" "$(1)" "$(2)"
export V


###############################################
#  build rules                                #
###############################################

help_target ::
	@echo "all    - build all target"
	@echo "install - install binaries"

help_variable ::
	@echo "DESTDIR - install under this dir rather "
	@echo "          then under /"
	@echo "DEBUG - compile with debug symbols, if non-null"

help_example ::
	@echo "@@"
	@echo "Installs stuff in a separate dir for easy packaging."
	@echo "  ./configure --prefix=/usr/local"
	@echo "  make"
	@echo "  make install DESTDIR=/tmp/tmp313231"

help_example ::
	@echo "@@"
	@echo "All standard make variables, like CFLAGS, LDFLAGS or CC, "
	@echo "are also supported."
	@echo "  make CFLAGS=-O2"
	@echo "  make CC=/opt/arm-gcc"

ifeq ($(origin CFLAGS),environment)
ifeq ($(CFLAGS_orig),)
override CFLAGS_orig := $(CFLAGS)
export CFLAGS_orig
else
override CFLAGS := $(CFLAGS_orig)
endif
endif

ifeq ($(origin CFLAGS),undefined)
CFLAGS = -O2
endif
ifneq ($(origin DEBUG),undefined)
override CFLAGS += -g
endif
override CFLAGS += -I$(TOPDIR) $(CFLAGS_EXTRA)

# Produce local obj name from C file path
src2base = $(basename $(notdir $(1)))

# Create rule to compile an object from source
# Parameters
# $1 - C file (eg ../some/dir/test.c)
# $2 - object file basename (eg test)
# $3 - target name (eg hello)
define objng_rules
ifneq ($(IS_ALL),)
-include $(2).d
endif
$(3)_obj += $(2).o
CLEANLIST += $(2).o $(2).d
$(2).o : $(1) $(TOPDIR)/config.h
	$(call summary,CC    ,$$@)
	$Q$(CC) $(CFLAGS) $($(3)_cflags) -c -o $$@ $$< -MMD $(out)
	$Qsed -i -e 's/\s\/\S\+/ /g' $(2).d
endef

define bin_rules
all : $(1)
CLEANLIST += $(1)

$(foreach s,$($(1)_src),\
	$(eval $(call objng_rules,$(s),$(call src2base,$(s)),$(1))))

$(1) : $($(1)_obj)
	$(call summary,BIN   ,$$@)
	$Q$(CC) $$^ $(LDFLAGS) $($(1)_libs) -o $$@ $(out)

ifeq ($($(1)_install),)
install : $(1)_install
$(1)_install :
	$Qinstall -D -m 755 -T $(1) $(DESTDIR)$(BINDIR)/$(1)
endif
endef


define lib_rules
all : lib$(1).so
CLEANLIST += lib$(1).so
$(eval $(1)_cflags += -fPIC)

$(foreach s,$($(1)_src),\
	$(eval $(call objng_rules,$(s),$(call src2base,$(s)),$(1))))

lib$(1).so :  $($(1)_obj)
	$(call summary,LIB   ,$$@)
	$Q$(CC) $$^ $(LDFLAGS) $($(1)_libs) -shared -o $$@ $(out)

ifeq ($($(1)_install),)
install : $(1)_install
$(1)_install :
	$Qinstall -D -m 755 -T lib$(1).so $(DESTDIR)$(LIBDIR)/lib$(1).so
endif
endef


define ar_rules
all : lib$(1).a
CLEANLIST += lib$(1).a


$(foreach s,$($(1)_src),\
	$(eval $(call objng_rules,$(s),$(call src2base,$(s)),$(1))))

lib$(1).a : $($(1)_obj)
	$(call summary,AR    ,$$@)
	$Q$(AR) rcs $$@ $$^

ifeq ($($(1)_install),)
install : $(1)_install
$(1)_install :
	$Qinstall -D -m 644 -T lib$(1).a $(DESTDIR)$(LIBDIR)/lib$(1).a

endif
endef

ifeq ($(KVERSION),)
KVERSION := $(shell uname -r)
#export KVERSION
endif

define kmod_rules
CLEANLIST += *.ko *.o *.mod.c .*.cmd .tmp_versions modules.order Module.symvers
all:
	$(call summary,KMOD  ,$(1).ko)
	$Qexport KBUILD_EXTRA_SYMBOLS=$(KB_SYMBOLS); \
	make -C /lib/modules/$$(KVERSION)/build M=$(CURDIR) \
		modules $(out)
# clean:
# 	$Qmake -C /lib/modules/$$(KVERSION)/build M=$(CURDIR) \
# 		clean $(out)

install:
	$Qmake -C /lib/modules/$$(KVERSION)/build M=$(CURDIR) \
		modules_install $(out)
endef

% : %.in
	@echo "TEXT    $@"
	$Q$(TOPDIR)/repl.py < $^ > $@


targets = $(filter %_type,$(.VARIABLES))
$(foreach t,$(targets),$(eval $(call $(strip $($(t)))_rules,$(t:_type=))))


###############################################
#  clean                                      #
###############################################

help_target ::
	@echo "clean - clean build results"
	@echo "distclean - clean build and configure results"

ifeq ($(IS_TOPDIR),yes)
DISTCLEANLIST += config.mk config.h repl.py .config/*.pyc
endif

clean:
ifneq (,$(CLEANLIST))
	$(call summary,CLEAN  ,)
	$Qrm -rf $(CLEANLIST)
endif

distclean : clean
ifneq (,$(DISTCLEANLIST))
	$(call summary,DCLEAN  ,)
	$Qrm -rf $(DISTCLEANLIST)
endif


###############################################
#  tar                                        #
###############################################

ifeq ($(IS_TOPDIR),yes)
help_target ::
	@echo "tar - make tar archive of a project code"

tar :
	$(call summary,TAR   ,/tmp/$(NV).tar.bz2)
	$Q$(TOPDIR)/.config/tar.py $(if $V,-v) /tmp/$(NV).tar.bz2
endif


###############################################
#  misc                                       #
###############################################

help_target ::
	@echo "svnignore - tell svn to ignore files in a cleanlist"

svnignore:
	@prop=prop-$$$$.txt; \
	for i in $(DISTCLEANLIST) $(CLEANLIST); do echo "$$i"; done > $$prop;  \
	cat $$prop; \
	svn propset svn:ignore --file $$prop .; \
	rm -f $$prop

