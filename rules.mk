#
#	rules.mk			(C) 2007-2008, Aurélien Croc (AP²C)
#
#  Compilation rules file for SpliX
#

$(rastertoqpdl_TARGET): $(rastertoqpdl_OBJ)
	$(call printCmd, $(cmd_link))
	$(Q)g++ -o $@ $^ $(rastertoqpdl_CXXFLAGS) $(rastertoqpdl_LDFLAGS) \
		$(rastertoqpdl_LIBS)

$(pstoqpdl_TARGET): $(pstoqpdl_OBJ)
	$(call printCmd, $(cmd_link))
	$(Q)g++ -o $@ $^ $(pstoqpdl_CXXFLAGS) $(pstoqpdl_LDFLAGS) \
		$(pstoqpdl_LIBS)

.PHONY: tags optionList
tags:
	ctags --recurse --language-force=c++ --extra=+q --fields=+i \
	      --exclude=doc --exclude=.svn * 


ifneq ($(DISABLE_JBIG),0)
JBIGSTATE := disabled
else
JBIGSTATE := enabled
endif
ifneq ($(DISABLE_THREADS),0)
THREADSSTATE := disabled
else
THREADSSTATE := enabled
endif
ifneq ($(DISABLE_BLACKOPTIM),0)
BLACKOPTIMSTATE := disabled
else
BLACKOPTIMSTATE := enabled
endif

optionList:
	@printf "   +---------------------------------------------+\n"
	@printf "   |      COMPILATION PARAMETERS SUMMARY         |\n"
	@printf "   +---------------------------------------------+\n"
	@printf "   |      THREADS     = %8s                 |\n" $(THREADSSTATE)
	@printf "   |      THREADS #   = %8i                 |\n" $(THREADS)
	@printf "   |      CACHESIZE   = %8i                 |\n" $(CACHESIZE)
	@printf "   |      JBIG        = %8s                 |\n" $(JBIGSTATE)
	@printf "   |      BLACK OPTIM = %8s                 |\n" $(BLACKOPTIMSTATE)
	@printf "   +---------------------------------------------+\n"
	@printf "  (Do a \"make clean\" before updating these values)\n"
	@echo ""
