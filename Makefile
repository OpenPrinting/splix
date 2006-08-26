#
# 	Makefile			(C) 2006, Aurélien Croc (AP²C)
#
#  This project has been placed under the GPL Licence.
#

CXXFLAGS	:= -O2
LDFLAGS		:= 
CUPSPREFIX	:= /usr
CUPSFILTER	:= lib/cups/filter
CUPSPPD		:= share/cups/model

# === DON'T CHANGE ANYTHNG AFTER THIS MESSAGE ====

export CXXFLAGS LDFLAGS CUPSPREFIX CUPSFILTER CUPSPPD

all: src ppd 

.PHONY: src ppd 
src ppd:
	@$(MAKE) -C $@

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
	@$(MAKE) -C ppd install
	@echo ""
	@echo "             --- Everything is done! Have fun ---"
	@echo ""
