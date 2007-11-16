#
# 	Makefile			(C) 2006, Aurélien Croc (AP²C)
#
#  This project has been placed under the GPL Licence.
#
#

SOURCE		:= samsung.drv dell.drv xerox.drv
DELL		:= 1100 1110
SAMSUNG		:= clp200 clp300 clp500 clp510 clp600 clp610 clx2170 clx3160 \
		   ml1510 ml1520 ml1610 ml1630 ml1710 ml1740 ml1750 ml2010 \
		   ml2150 ml2250 ml2510 ml2550 ml3050 ml3560
XEROX		:= ph3115 ph3116 ph3117 ph3120 ph3121 ph3122 ph3130 ph3150 \
		   ph3420 ph3425 ph5500 ph6100 ph6110
DRIVERS		:= $(DELL) $(SAMSUNG) $(XEROX)
DRIVERSEXT	:= ppd
POEXT		:= po
PODIR		:= po
LANGUAGES 	:= fr it de

# === DON'T CHANGE ANYTHING AFTER THIS MESSAGE ====

MASTERDRIVER	:= $(shell echo "${DRIVERS}" | awk '{ print $$1 }')
DRIVER		:= $(MASTERDRIVER).$(DRIVERSEXT)
LANGDRIVERS	:= $(foreach name, $(LANGUAGES), $(MASTERDRIVER)$(name))
LANGDRIVERSEXT	:= $(addsuffix .$(DRIVERSEXT), $(LANGDRIVERS))

all:

ppd: $(DRIVER) $(LANGDRIVERSEXT)

$(DRIVER): $(SOURCE)
	./compile.sh samsung.drv -I . -d ./
	./compile.sh dell.drv -I . -d ./
	./compile.sh xerox.drv -I . -d ./

$(LANGDRIVERSEXT): $(SOURCE) $(patsubst %, $(PODIR)/%.$(POEXT), $(LANGUAGES))
	lang=$(patsubst $(MASTERDRIVER)%.$(DRIVERSEXT),%, $@); \
	./compile.sh samsung.drv -c ${PODIR}/$$lang.${POEXT} -l $$lang -d ${PODIR}/$$lang; \
	for filename in ${SAMSUNG}; do \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done; \
	./compile.sh xerox.drv -c ${PODIR}/$$lang.${POEXT} -l $$lang -d ${PODIR}/$$lang; \
	for filename in ${XEROX}; do \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done; \
	./compile.sh dell.drv -c ${PODIR}/$$lang.${POEXT} -l $$lang -d ${PODIR}/$$lang; \
	for filename in ${DELL}; do \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done;

.PHONY: update
update: $(patsubst %, $(PODIR)/%.$(POEXT), $(LANGUAGES))
%.po: $(SOURCE)
	./compile.sh samsung.drv lang $@
	./compile.sh dell.drv lang $@
	./compile.sh xerox.drv lang $@

.PHONY: install
install:
	install -d -m 755 ${CUPSPPD}/samsung
	for filename in ${SAMSUNG}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${CUPSPPD}/samsung;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${CUPSPPD}/samsung;\
		done; \
	done; \
	install -d -m 755 ${CUPSPPD}/xerox
	for filename in ${XEROX}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${CUPSPPD}/samsung;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${CUPSPPD}/samsung;\
		done; \
	done; \
	install -d -m 755 ${CUPSPPD}/dell
	for filename in ${DELL}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${CUPSPPD}/samsung;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${CUPSPPD}/samsung;\
		done; \
	done;

.PHONY: clean distclean
clean:
distclean:
	$(RM) *.${DRIVERSEXT}
