#
# 	Makefile			(C) 2006, Aurélien Croc (AP²C)
#
#  This project has been placed under the GPL Licence.
#
#

SOURCE		:= samsung.drv
DRIVERS		:= ml1710 ml2010 ml2250
DRIVERSEXT	:= ppd
POEXT		:= po
PODIR		:= po
LANGUAGES 	:= fr

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
	for filename in ${DRIVERS}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${CUPSPREFIX}/${CUPSPPD}/samsung;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${CUPSPREFIX}/${CUPSPPD}/samsung;\
		done; \
	done \

.PHONY: clean distclean
clean:
distclean:
	$(RM) *.${DRIVERSEXT}
