TOPDIR = ..
include $(TOPDIR)/Makefile.common

SRC = ../shots

all: $(SRC).html 

$(SRC).thtml: $(wildcard */* */) ./mk_gallery 
	./mk_gallery $@

clean: clean2

clean2:
	$(RM) $(SRC).thtml