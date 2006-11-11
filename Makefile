#
# 	Makefile			(C) 2006, Aurélien Croc (AP²C)
#
#  This project has been placed under the GPL Licence.
#
#

SOURCE		:= samsung.drv
DRIVERS		:= ml1510 ml1520 ml1610 ml1710 ml1740 ml1750 ml2010 ml2150 ml2250 ml2550 clp300 clp500 clp510 clp600
DRIVERSEXT	:= ppd
POEXT		:= po
PODIR		:= po
LANGUAGES 	:= fr it de

# === DON'T CHANGE ANYTHING AFTER THIS MESSAGE ====

MASTERDRIVER	:= $(shell echo "${DRIVERS}" | awk '{ print $$1 }')
DRIVER		:= $(MASTERDRIVER).$(DRIVERSEXT)
LANGDRIVERS	:= $(foreach name, $(LANGUAGES), $(MASTERDRIVER)$(name))
LANGDRIVERSEXT	:= $(addsuffix .$(DRIVERSEXT), $(LANGDRIVERS))

all: $(DRIVER) $(LANGDRIVERSEXT)

$(DRIVER): $(SOURCE)
	ppdc -d ./ $<

$(LANGDRIVERSEXT): $(SOURCE) $(patsubst %, $(PODIR)/%.$(POEXT), $(LANGUAGES))
	ppdc -c ${PODIR}/$(patsubst $(MASTERDRIVER)%.$(DRIVERSEXT),%, $@).${POEXT} -l $(patsubst $(MASTERDRIVER)%.$(DRIVERSEXT),%, $@) -d ${PODIR} $<
	for filename in ${DRIVERS}; do \
		mv ${PODIR}/`echo $$filename`.${DRIVERSEXT} `echo $$filename`$(patsubst $(MASTERDRIVER)%.$(DRIVERSEXT),%, $@).${DRIVERSEXT}; \
	done

.PHONY: update
update: $(patsubst %, $(PODIR)/%.$(POEXT), $(LANGUAGES))
%.po: $(SOURCE)
	ppdpo -o $@ $<

.PHONY:
install:
	install -d -m 755 ${CUPSPPD}/samsung
	for filename in ${DRIVERS}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${CUPSPPD}/samsung;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${CUPSPPD}/samsung;\
		done; \
	done \

.PHONY: clean distclean
clean:
distclean:
	$(RM) *.${DRIVERSEXT}
