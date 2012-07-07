#
# 	Makefile			(C) 2006, Aurélien Croc (AP²C)
#
#  This project has been placed under the GPL Licence.
#
#

SOURCE		:= samsung.drv.in dell.drv.in xerox.drv.in lexmark.drv.in toshiba.drv.in
DRVS		:= samsung.drv dell.drv xerox.drv lexmark.drv toshiba.drv
DELL		:= 1100 1110
SAMSUNG		:= clp500 clp550 clp510 ml1510 ml1520 ml1610 \
		   ml1630 ml1640 ml1660 ml1710 ml1740 ml1750 ml1910 ml1915 \
                   ml2010 ml2015 ml2150 ml2240 ml2250 ml2251 ml2510 ml2525 \
                   ml2525w ml2550 ml2571 ml2580 ml2580n ml3050 ml3051 \
		   ml3051nd ml3471nd ml3560 scx3200 scx4100 scx4200 scx4216f \
		   scx4300 scx4500 scx4521f scx4600 scx4623f scx4623fw \
		   scx5330n scx5530fn sf565p
#SAMSUNG		+= clp610 clp610nd
XEROX		:= ph3115 ph3116 ph3117 ph3120 ph3121 ph3122 ph3124 ph3130 \
		   ph3140 ph3150 ph3155 ph3160 ph3420 ph3425 ph5500 ph6100 \
		   wc3119 wcpe16 wcpe114e
LEXMARK		:= x215mfp
TOSHIBA		:= es180s
DRIVERSEXT	:= ppd
POEXT		:= po
PODIR		:= po
LANGUAGES 	:= fr pt
# === MAKE ALL PPD BY DEFAULT ====
ifneq ($(DISABLE_JBIG),1)
SAMSUNG 	+= clp200 clp300 clp310 clp315 clp310n clp600 clx216x clx2170 \
                   clx3160
XEROX		+= ph6110
endif
DRIVERS		:= $(DELL) $(SAMSUNG) $(XEROX) $(LEXMARK) $(TOSHIBA)

# === DON'T CHANGE ANYTHING AFTER THIS MESSAGE ====

MASTERDRIVER	:= $(shell echo "${DRIVERS}" | awk '{ print $$1 }')
DRIVER		:= $(MASTERDRIVER).$(DRIVERSEXT)
LANGDRIVERS	:= $(foreach name, $(LANGUAGES), $(MASTERDRIVER)$(name))
LANGDRIVERSEXT	:= $(addsuffix .$(DRIVERSEXT), $(LANGDRIVERS))
printCmd        = $(if $(filter $(V),1),,$(shell echo "@echo \"    $(1)\""))


all:

.PHONY: drv
drv: $(SOURCE)
	./compile.sh samsung.drv.in drv
	./compile.sh dell.drv.in drv
	./compile.sh xerox.drv.in drv
	./compile.sh lexmark.drv.in drv
	./compile.sh toshiba.drv.in drv

.PHONY: ppd
ppd: $(DRIVER) $(LANGDRIVERSEXT)

$(DRIVER): $(SOURCE)
	./compile.sh samsung.drv.in -I . -d ./
	./compile.sh dell.drv.in -I . -d ./
	./compile.sh xerox.drv.in -I . -d ./
	./compile.sh lexmark.drv.in -I . -d ./
	./compile.sh toshiba.drv.in -I . -d ./

$(LANGDRIVERSEXT): $(SOURCE) $(patsubst %, $(PODIR)/%.$(POEXT), $(LANGUAGES))
	lang=$(patsubst $(MASTERDRIVER)%.$(DRIVERSEXT),%, $@); \
	./compile.sh samsung.drv.in -l $$lang -c ${PODIR}/$$lang.${POEXT} -d ${PODIR}/$$lang; \
	for filename in ${SAMSUNG}; do \
		recode utf8..latin1 ${PODIR}/$$lang/$$filename.${DRIVERSEXT}; \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done; \
	./compile.sh xerox.drv.in -l $$lang -c ${PODIR}/$$lang.${POEXT} -d ${PODIR}/$$lang; \
	for filename in ${XEROX}; do \
		recode utf8..latin1 ${PODIR}/$$lang/$$filename.${DRIVERSEXT}; \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done; \
	./compile.sh dell.drv.in -l $$lang -c ${PODIR}/$$lang.${POEXT} -d ${PODIR}/$$lang; \
	for filename in ${DELL}; do \
		recode utf8..latin1 ${PODIR}/$$lang/$$filename.${DRIVERSEXT}; \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done; \
	./compile.sh lexmark.drv.in -l $$lang -c ${PODIR}/$$lang.${POEXT} -d ${PODIR}/$$lang; \
	for filename in ${LEXMARK}; do \
		recode utf8..latin1 ${PODIR}/$$lang/$$filename.${DRIVERSEXT}; \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done; \
	./compile.sh toshiba.drv.in -l $$lang -c ${PODIR}/$$lang.${POEXT} -d ${PODIR}/$$lang; \
	for filename in ${TOSHIBA}; do \
		recode utf8..latin1 ${PODIR}/$$lang/$$filename.${DRIVERSEXT}; \
		mv ${PODIR}/$$lang/$$filename.${DRIVERSEXT} $$filename$$lang.${DRIVERSEXT}; \
	done;

.PHONY: update
update: $(patsubst %, $(PODIR)/%.$(POEXT), $(LANGUAGES))
%.po: $(SOURCE)
	./compile.sh samsung.drv.in lang $@
	./compile.sh dell.drv.in lang $@
	./compile.sh xerox.drv.in lang $@
	./compile.sh lexmark.drv.in lang $@
	./compile.sh toshiba.drv.in lang $@

cmd_install_samsung	= INSTALL           Samsung PPD files
cmd_install_xerox	= INSTALL           Xerox PPD files
cmd_install_dell	= INSTALL           Dell PPD files
cmd_install_lexmark	= INSTALL           Lexmark PPD files
cmd_install_toshiba	= INSTALL           Toshiba PPD files
cmd_install_drv		= INSTALL           DRV files
.PHONY: install
install:
ifneq  ($(DRV_ONLY),1)
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
	$(call printCmd, $(cmd_install_lexmark))
	$(Q)install -d -m 755 ${DESTDIR}${CUPSPPD}/lexmark
	$(Q)for filename in ${LEXMARK}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/lexmark;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/lexmark;\
		done; \
	done;
	$(call printCmd, $(cmd_install_toshiba))
	$(Q)install -d -m 755 ${DESTDIR}${CUPSPPD}/toshiba
	$(Q)for filename in ${TOSHIBA}; do \
		install -m 644 $$filename.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/toshiba;\
		for lang in ${LANGUAGES}; do \
			install -m 644 $$filename$$lang.${DRIVERSEXT} ${DESTDIR}${CUPSPPD}/toshiba;\
		done; \
	done;
else
	$(call printCmd, $(cmd_install_drv))
	$(Q)install -d -m 755 ${DESTDIR}${CUPSDRV}
	$(Q)for filename in ${DRVS}; do \
		install -m 644 $$filename ${DESTDIR}${CUPSDRV};\
		done;
endif

.PHONY: clean distclean
clean:
distclean:
	$(RM) *.${DRIVERSEXT}
	$(RM) *.drv
