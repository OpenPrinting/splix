#
# 	Makefile			(C) 2006, Aurélien Croc (AP²C)
#
#  This project has been placed under the GPL Licence.
#

CXXFLAGS	:= -O2 `cups-config --cflags` 
LDFLAGS		:= `cups-config --ldflags`
CUPSFILTER	:= `cups-config --serverbin`/filter
CUPSPPD		:= `cups-config --datadir`/model

# === DON'T CHANGE ANYTHING AFTER THIS MESSAGE ====

export CXXFLAGS LDFLAGS CUPSFILTER CUPSPPD

all: src ppd 

.PHONY: src ppd 
src ppd:
	@$(MAKE) -C $@ DISABLE_JBIG=$(DISABLE_JBIG)

.PHONY: clean distclean
clean:
	@$(MAKE) -C src clean
	@$(MAKE) -C ppd clean

distclean: clean
	@$(MAKE) -C src distclean
	@$(MAKE) -C ppd distclean

.PHONY: install
install: 
	@$(MAKE) -C src install
	@$(MAKE) -C ppd install DISABLE_JBIG=$(DISABLE_JBIG)
	@echo ""
	@echo "             --- Everything is done! Have fun ---"
	@echo ""
