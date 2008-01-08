#
#	Makefile		(C) 2006, Aurélien Croc (AP²C)
#
# This project and this Makefile are under the GPL v2 Licence
# 
# Available variables in module.mk :
#      * SUBDIRS
#      * MODE = {default,debug,optimized}
#      * CFLAGS
#      * CXXFLAGS
#      * LDFLAGS
#      * DEBUG_CFLAGS
#      * DEBUG_CXXFLAGS
#      * OPTIMIZED_CFLAGS
#      * OPTIMIZED_CXXFLAGS
#      * TARGETS
#      * FLAGS_directory1_directory2_file_ext
#
# Each project must be listed in the TARGETS variable. Next:
#      * project1_SRC
#      * project1_MODULES
#      * project1_LIB
#      * ...
#
# This Makefile will generate:
#      * project1_TARGET
#      * project1_OBJ
#
#
# To add a new target file support, please add the target extension in the
# TARGET_RULES variable and define the function targetDefinition_{extension}.
# This function will be called with three parameters:
# 	1. the target file
# 	2. the objects files
# 	3. the target name to have access to others functions (by adding _SRC,
# 		_LIB, _MODULES, ...)
#
#
# /!\ = Please do your modifications in the module.mk file = /!\
# 
# $Id$
#


# +--------------------------------------------------------------------------+
# |   SUPPORTED LANGUAGES, DIRECTORIES AND TOOLS LOCATIONS & FLAGS           |
# +--------------------------------------------------------------------------+

LANGUAGES	:= cpp c
COMPILED_LANG	:= cpp c

DEPDIR		:= .deps
BUILDDIR	:= .build
TARGETDIR	:= .

CC		:= gcc
CXX		:= g++
RM		:= rm -f
AR		:= ar crs

empty		:=
space		:= $(empty) $(empty)
comma		:= ,

DEBUG_CFLAGS	:= -O0 -g
DEBUG_CXXFLAGS	:= -O0 -g
OPTIMIZED_CFLAGS	:= -O2
OPTIMIZED_CXXFLAGS	:= -O2




# +--------------------------------------------------------------------------+
# |   DEFINITIONS VARIABLE LOADING                                           |
# +--------------------------------------------------------------------------+

MODE		:= default
DEFFILE		:= .defs.mk
-include $(DEFFILE)




# +--------------------------------------------------------------------------+
# |   COMPILATION MODE INITIALIZATION                                        |
# +--------------------------------------------------------------------------+

ifeq ($(MAKECMDGOALS),debug)
MODE		:= debug
endif
ifeq ($(MAKECMDGOALS),optimized)
MODE		:= optimized
endif
ifeq ($(MAKECMDGOALS),default)
MODE		:= default
endif

ifeq ($(MODE),debug) 		# DEBUG
_CFLAGS += $(_DEBUG_CFLAGS)
_CXXFLAGS += $(_DEBUG_CXXFLAGS)
BUILDDIR = debug
TARGETDIR = debug
DEPDIR = debug
else
ifeq ($(MODE),optimized)	# OPTIMIZED
_CFLAGS += $(_OPTIMIZED_CFLAGS)
_CXXFLAGS += $(_OPTIMIZED_CXXFLAGS)
BUILDDIR = optimized
TARGETDIR = optimized
DEPDIR = optimized
endif
endif




# +--------------------------------------------------------------------------+
# |   VERBOSE MODE AND INITIALIZATION                                        |
# +--------------------------------------------------------------------------+

V := 
ifeq ($(V),1) 
	Q := 
else
	Q := @
endif




# +--------------------------------------------------------------------------+
# |   MAIN RULES AND TARGETS                                                 |
# +--------------------------------------------------------------------------+

__TARGETS	:= $(foreach target,$(_TARGETS),$(TARGETDIR)/$(target))

all: $(__TARGETS)
debug: $(__TARGETS)
optimized: $(__TARGETS)




# +--------------------------------------------------------------------------+
# |   OBJECTS AND TARGETS DEFINITIONS                                        |
# +--------------------------------------------------------------------------+

# Get the target name variable
targetName	= $(subst .,_,$(subst /,_,$(1)))

# Generate each TARGETNAME_OBJ and TARGETNAME_TARGET
define defineObj
$(1)_OBJ	= $(foreach obj,$(foreach lang,$(COMPILED_LANG),$(patsubst \
		   %.$(lang), %.o, $(filter %.$(lang),$(value _$(1)_SRC)))), \
		   $(BUILDDIR)/$(obj)) $(foreach module,$(value _$(1)_MODULES),\
		   $(TARGETDIR)/$(module))
endef
define defineTarget
$(call targetName,$(1))_TARGET	= $(TARGETDIR)/$(1)
endef
$(foreach target,$(_TARGETS),$(eval $(call defineObj,$(strip $(call \
	targetName,$(target))))) $(eval $(call defineTarget,$(strip \
	$(target)))))




# +--------------------------------------------------------------------------+
# |   SUBDIRS DEFINITIONS                                                    |
# +--------------------------------------------------------------------------+

include module.mk
include $(patsubst %, %/module.mk, $(_SUBDIRS))




# +--------------------------------------------------------------------------+
# |   SOURCE AND DEPENDENCIES FILES                                          |
# +--------------------------------------------------------------------------+

# Get all the source files in ALLSRC
modulevalue	 = $(value $(subst /,_,$(basename $(1))))
modulesrc	 = $(foreach module, $(value $(1)_MODULES), $(call modulevalue,\
			$(module)))
ALLSRC		:= $(foreach target,$(call targetName,$(_TARGETS)),$(value \
			$(target)_SRC) $(call modulesrc,$(target)))

# Get the dependencies files
DEPENDENCIES	:= $(foreach lang,$(LANGUAGES),$(patsubst %.$(lang), %.d, \
		   $(filter %.$(lang),$(ALLSRC))))




# +--------------------------------------------------------------------------+
# |   TARGET RULES                                                           |
# +--------------------------------------------------------------------------+

TARGET_RULES	:= a
define targetDefinition_a
$(1): $(2)
	$$(call printCmd, $$(cmd_ar_a_o))
	$$(Q)$$(AR) $$@ $$^
$(call defineObj,$(3))
endef
-include rules.mk

$(foreach target,$(_TARGETS),$(if $(filter $(TARGET_RULES),$(subst \
	.,,$(suffix $(target)))), $(eval $(call \
	targetDefinition_$(subst .,,$(suffix \
	$(target))),$(value $(call targetName,$(target)_TARGET)),$(value \
	$(call targetName,$(target))_OBJ),$(call targetName,$(target))))))




# +--------------------------------------------------------------------------+
# |   COMPILATION AND CLEAN RULES                                            |
# +--------------------------------------------------------------------------+

# Function to print smart messages
printCmd	 = $(if $(filter $(V),1),,$(shell echo "@echo \"    $(1)\""))

# Smart messages
cmd_ar_a_o	 = AR                $@
cmd_cc_o_c	 = CC                $<
cmd_cxx_o_cpp	 = CXX               $<
cmd_link	 = LINK              $@
cmd_rm_clean	:= RM                *.o *.a
cmd_rm_distclean = RM                $(__TARGETS) *.d .defs.mk
cmd_create_defs  = UPDATE            $(DEFFILE)

# Specific flags definition
flags		 = $(value FLAGS_$(subst /,_,$(subst .,_,$(1))))

# C Files
$(BUILDDIR)/%.o: %.c
	$(call printCmd, $(cmd_cc_o_c))
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(_CFLAGS) $(call flags,$<) -o $@ -c $<

# C++ Files
$(BUILDDIR)/%.o: %.cpp
	$(call printCmd, $(cmd_cxx_o_cpp))
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(_CXXFLAGS) $(call flags,$<) -o $@ -c $<

# Clean and distclean rules
.PHONY: clean distclean
clean:
	$(call printCmd, $(cmd_rm_clean))
	$(Q)$(RM) $(foreach obj,$(DEPENDENCIES:.d=.o),$(BUILDDIR)/$(obj))
	$(Q)$(RM) $(foreach dir,$(_MODULES),$(BUILDDIR)/$(dir))

distclean: clean
	$(call printCmd, $(cmd_rm_distclean))
	$(Q)$(RM) $(foreach dep,$(DEPENDENCIES),$(DEPDIR)/$(dep))
	$(Q)$(RM) $(__TARGETS)
	$(Q)$(RM) $(DEFFILE)




# +--------------------------------------------------------------------------+
# |   DEPENDENCIES MANAGMENT                                                 |
# +--------------------------------------------------------------------------+

# Include the include dependencies files
-include $(foreach dep,$(DEPENDENCIES),$(DEPDIR)/$(dep))

# Generate dependencies files for C++ source file
$(DEPDIR)/%.d: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(_CXXFLAGS) -MM -MG -MT "\$$(DEPDIR)/$(patsubst %.cpp,%.d,\
	$<) \$$(BUILDDIR)/$(patsubst %.cpp,%.o,$<)" -MG "$<" -MF $@

# Generate dependencies files for C source file
$(DEPDIR)/%.d: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(_CFLAGS) -MM -MG -MT "\$$(DEPDIR)/$(patsubst %.c,%.d,\
	$<) \$$(BUILDDIR)/$(patsubst %.c,%.o,$<)" -MG "$<" -MF $@

# Generate the defs.mk file which contains global variables
#$(call printCmd, $(cmd_create_defs))
$(DEFFILE): Makefile $(patsubst %, %/module.mk, $(SUBDIRS)) module.mk
	@echo "" > $@
	@make -s -C ./ _depsreload


.PHONY: _depsreload
_depsreload:
	@echo "MODE := $(MODE)" > $(DEFFILE)
	@echo "_SUBDIRS := $(SUBDIRS)" >> $(DEFFILE)
	@echo "_CFLAGS := $(CFLAGS)" >> $(DEFFILE)
	@echo "_CXXFLAGS := $(CXXFLAGS)" >> $(DEFFILE)
	@echo "_LDFLAGS := $(LDFLAGS)" >> $(DEFFILE)
	@echo "_TARGETS := $(TARGETS)" >> $(DEFFILE)
	@echo "_DEBUG_CFLAGS := $(DEBUG_CFLAGS)" >> $(DEFFILE)
	@echo "_DEBUG_CXXFLAGS := $(DEBUG_CXXFLAGS)" >> $(DEFFILE)
	@echo "_OPTIMIZED_CFLAGS := $(OPTIMIZED_CFLAGS)" >> $(DEFFILE)
	@echo "_OPTIMIZED_CXXFLAGS := $(OPTIMIZED_CXXFLAGS)" >> $(DEFFILE)
	@echo -e $(foreach target,$(call targetName,$(TARGETS)),\
		"_$(target)_SRC := "\
		"$(value $(target)_SRC)\n") >> $(DEFFILE)
	@echo -e $(foreach target,$(call targetName,$(TARGETS)),\
		"_$(target)_LIBS := "\
		"$(value $(target)_LIBS)\n") >> $(DEFFILE)
	@echo -e $(foreach target,$(call targetName,$(TARGETS)),\
		"_$(target)_MODULES := "\
		"$(value $(target)_MODULES)\n") >> $(DEFFILE)
	@if [ "$(SUBDIRS)" != "$(_SUBDIRS)" ]; then make -s -C ./ _depsreload; fi

