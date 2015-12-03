TOPDIR = ..
include $(TOPDIR)/Makefile.common

SRC = ../plugins

all: $(SRC).html 

$(SRC).thtml: $(wildcard */* */) ./mk_gallery 
#./mk_gallery $@
	@echo "coming soon..." > $@

clean: clean2

clean2:
	$(RM) $(SRC).thtml
