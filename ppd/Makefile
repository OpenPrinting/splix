#
# 	Makefile			(C) 2006, Aurélien Croc (AP²C)
#
#  This project has been placed under the GPL Licence.
#
#

SOURCE		:= samsung.drv dell.drv xerox.drv
DELL		:= 1100 1110
SAMSUNG		:= clp500 clp550 clp510 clp610 clp610nd ml1510 ml1520 ml1610 \
		   ml1630 ml1640 ml1710 ml1740 ml1750 ml2010 ml2015 ml2150 \
		   ml2240 ml2250 ml2251 ml2510 ml2550 ml2571 ml3050 ml3051 \
		   ml3051nd ml3560 scx4100 scx4200 scx4300 scx4500 sf565p
XEROX		:= ph3115 ph3116 ph3117 ph3120 ph3121 ph3122 ph3124 ph3130 \
		   ph3150 ph3160 ph3420 ph3425 ph5500 ph6100 wc3119
DRIVERSEXT	:= ppd
POEXT		:= po
PODIR		:= po
LANGUAGES 	:= fr pt
ifeq ($(DISABLE_JBIG),0)
SAMSUNG 	+= clp200 clp300 clp600 clx216x clx2170 clx3160
XEROX		+= ph6110
endif
DRIVERS		:= $(DELL) $(SAMSUNG) $(XEROX)

# === DON'T CHANGE ANYTHING AFTER THIS MESSAGE ====

MASTERDRIVER	:= $(shell echo "${DRIVERS}" | awk '{ print $$1 }')
DRIVER		:= $(MASTERDRIVER).$(DRIVERSEXT)
LANGDRIVERS	:= $(foreach name, $(LANGUAGES), $(MASTERDRIVER)$(name))
LANGDRIVERSEXT	:= $(addsuffix .$(DRIVERSEXT), $(LANGDRIVERS))
printCmd        = $(if $(filter $(V),1),,$(shell echo "@echo \"    $(1)\""))


all:

.PHONY: ppd
ppd: $(DRIVER) $(LANGDRIVERSEXT)

$(DRIVER): $(SOURCE)
	./compile.sh samsung.drv -I . -d ./
	./compile.sh dell.drv -I . -d ./
	./compile.sh xerox.drv -I . -d ./

$(LANGDRIVERSEXT): $(SOURCE) $(patsubst %, $(PODIR)/%.$(POEXT), $(LANGUAGES))
	lang=$(patsubst $(MASTERDRIVER)%.$(DRIVERSEXT),%, $@); \
	./compile.sh samsung.drv -l $$lang -c ${PODIR}/$$lang.${POEXT} -d ${PODIR}/$$lang; \
	for filename in ${SAMSUNG}; do \
		recode utf8..latin1 ${PODIR}/$$lang/$$filename.${DRIVERSEXT}; \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done; \
	./compile.sh xerox.drv -l $$lang -c ${PODIR}/$$lang.${POEXT} -d ${PODIR}/$$lang; \
	for filename in ${XEROX}; do \
		recode utf8..latin1 ${PODIR}/$$lang/$$filename.${DRIVERSEXT}; \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done; \
	./compile.sh dell.drv -l $$lang -c ${PODIR}/$$lang.${POEXT} -d ${PODIR}/$$lang; \
	for filename in ${DELL}; do \
		recode utf8..latin1 ${PODIR}/$$lang/$$filename.${DRIVERSEXT}; \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done;

.PHONY: update
update: $(patsubst %, $(PODIR)/%.$(POEXT), $(LANGUAGES))
%.po: $(SOURCE)
	./compile.sh samsung.drv lang $@
	./compile.sh dell.drv lang $@
	./compile.sh xerox.drv lang $@

cmd_install_samsung	= INSTALL           Samsung PPD files
cmd_install_xerox	= INSTALL           Xerox PPD files
cmd_install_dell	= INSTALL           Dell PPD files
.PHONY: install
install:
	$(call printCmd, $(cmd_install_samsung))
	$(Q)install -d -m 755 ${DESTDIR}${CUPSPPD}/samsung
	$(Q)for filename in ${SAMSUNG}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/samsung;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/samsung;\
		done; \
	done;
	$(call printCmd, $(cmd_install_xerox))
	$(Q)install -d -m 755 ${DESTDIR}${CUPSPPD}/xerox
	$(Q)for filename in ${XEROX}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/xerox;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/xerox;\
		done; \
	done;
	$(call printCmd, $(cmd_install_dell))
	$(Q)install -d -m 755 ${DESTDIR}${CUPSPPD}/dell
	$(Q)for filename in ${DELL}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/dell;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/dell;\
		done; \
	done;

.PHONY: clean distclean
clean:
distclean:
	$(RM) *.${DRIVERSEXT}
