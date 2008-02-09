#
#	Makefile		(C) 2006-2007, Aurélien Croc (AP²C)
#
# This project is under the GPL Licence
#
# Available MAKE options:
#      * V sets to 1 for verbose mode
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
#      * directory1_directory2_file_ext_FLAGS
#
# Each project must be listed in the TARGETS variable. Next:
#      * project1_SRC
#      * project1_LIB
#      * project1_CFLAGS
#      * project1_LDFLAGS
#      * project1_MODULES
#      * project1_CXXFLAGS
#      * ...
#
# This Makefile will generate:
#      * project1_TARGET
#      * project1_OBJ
#
#
# In the file rules.mk:
#
# You can add your own rules
# 
# To add a new target file support, please add the target extension in the
# TARGET_RULES variable and define the function targetDefinition_{extension}.
# This function will be called with one parameters: the target name
# Like the examples above, use $(value $(1)_TARGET) or _OBJ, .. to access
# to useful informations
#
#
# /!\ = Please do your modifications in the module.mk file = /!\
# 


# +--------------------------------------------------------------------------+
# |   SUPPORTED LANGUAGES, DIRECTORIES ARCHI AND TOOLS LOCATIONS & FLAGS     |
# +--------------------------------------------------------------------------+
LANGUAGES	:= cpp c

CC		:= gcc
CXX		:= g++
RM		:= rm -f
AR		:= ar crs
LEX		:= flex
YACC		:= bison
LINKER		:= $(CXX)

DEPDIR          := .deps
BUILDDIR        := .build
TARGETDIR       := .


empty           :=
space           := $(empty) $(empty)
comma           := ,

DEBUG_CFLAGS    := -O0 -g
DEBUG_CXXFLAGS  := -O0 -g
OPTIM_CFLAGS	:= -O2
OPTIM_CXXFLAGS	:= -O2

ARCHI           := $(shell uname -s)

ifeq ($(ARCHI),Darwin)
PLUGIN_EXT      := bundle
LIBRARY_EXT     := dylib
else
PLUGIN_EXT      := so
LIBRARY_EXT     := so
endif


# +--------------------------------------------------------------------------+
# |   DEFINITIONS VARIABLE LOADING                                           |
# +--------------------------------------------------------------------------+

MODE		:= default
DEFFILE		:= .defs.mk
-include $(DEFFILE)



# +--------------------------------------------------------------------------+
# |   SUBDIRS LOADING                                                        |
# +--------------------------------------------------------------------------+

include module.mk
ifeq ($(DEFLOADED),1)
include $(patsubst %, %/module.mk, $(_SUBDIRS))
endif # DEFLOADED == 1
ifeq ($(DEFDONE),1)




# +--------------------------------------------------------------------------+
# |   COMPILATION MODE INITIALIZATION                                        |
# +--------------------------------------------------------------------------+

ifeq ($(MAKECMDGOALS),debug)
MODE            := debug
endif #MAKECMDGOALS == debug
ifeq ($(MAKECMDGOALS),optimized)
MODE            := optimized
endif #MAKECMDGOALS == optimized
ifeq ($(MAKECMDGOALS),default)
MODE            := default
endif #MAKECMDGOALS == default

ifeq ($(MODE),debug)            # DEBUG
CFLAGS		+= $(DEBUG_CFLAGS)
CXXFLAGS	+= $(DEBUG_CXXFLAGS)
BUILDDIR	:= debug
TARGETDIR	:= debug
DEPDIR		:= debug
else
ifeq ($(MODE),optimized)        # OPTIMIZED
CFLAGS		+= $(OPTIM_CFLAGS)
CXXFLAGS	+= $(OPTIM_CXXFLAGS)
BUILDDIR	:= optimized
TARGETDIR	:= optimized
DEPDIR		:= optimized
endif # MODE == optimized
endif # MODE == debug



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

_TARGETS	:= $(foreach target,$(TARGETS),$(TARGETDIR)/$(target))

all: $(_TARGETS)
debug: $(_TARGETS)
optimized: $(_TARGETS)



# +--------------------------------------------------------------------------+
# |   MACRO DEFINITIONS                                                      |
# +--------------------------------------------------------------------------+

# Function to print smart messages
printCmd	= $(if $(filter $(V),1),,$(shell echo "@echo \"    $(1)\""))

# Get the target variable name
targetName	= $(subst .,_,$(subst /,_,$(1)))

# Specific flags definition
flags            = $(value $(subst /,_,$(subst .,_,$(1)))_FLAGS)
flag            = $(subst /,_,$(subst .,_,$(1)))_FLAGS



# +--------------------------------------------------------------------------+
# |   LOAD AND GENERATE DEPENDENCIES FILES                                   |
# +--------------------------------------------------------------------------+

# Get all the source files in ALLSRC
ALLSRC          := $(foreach target,$(call targetName,$(TARGETS)),$(value \
                        $(target)_SRC))

# Get the dependencies files
DEPENDENCIES    := $(foreach lang,$(LANGUAGES),$(patsubst %.$(lang), %.d, \
                   $(filter %.$(lang),$(ALLSRC))))

# Generate dependencies files for C++ source file
$(DEPDIR)/%.d: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -MM -MG -MT "\$$(DEPDIR)/$(basename $<).d \
		\$$(BUILDDIR)/$(basename $<).o" -MG "$<" -MF $@

# Load dependencies files 
-include $(foreach dep,$(DEPENDENCIES),$(DEPDIR)/$(dep))



# +--------------------------------------------------------------------------+
# |   PREVENT LOADING RULES IF DEFFILE IS NOT COMPLET                        |
# +--------------------------------------------------------------------------+

else
TARGETS	:= $(empty)

endif # DEFDONE == 1



# +--------------------------------------------------------------------------+
# |   OBJECTS AND TARGETS DEFINITIONS                                        |
# +--------------------------------------------------------------------------+


# Define target variables
defineTarget	= $(call targetName,$(1))_TARGET  := $(TARGETDIR)/$(1)

# Define target object variables
define defineObject
$(1)_OBJ	:= $(foreach obj,$(foreach lang,$(LANGUAGES),$(patsubst \
			%.$(lang),%.o,$(filter %.$(lang),$(value $(1)_SRC)))), \
			$(BUILDDIR)/$(obj)) \
		   $(foreach module,$(value \
			$(1)_MODULES),$(TARGETDIR)/$(module)) \
		   $(foreach obj,$(patsubst %.l,%.l.o,$(filter %.l,$(value \
			$(1)_SRC))),$(BUILDDIR)/$(obj)) \
		   $(foreach obj,$(patsubst %.y,%.yy.o,$(filter %.y,$(value \
			$(1)_SRC))),$(BUILDDIR)/$(obj))
$(1)_CLEAN	:= $(foreach obj,$(patsubst %.y,%.yy.h,$(filter %.y,$(value \
			$(1)_SRC))),$(BUILDDIR)/$(obj))
endef

# Create these definitions
$(foreach target,$(TARGETS),$(eval $(call defineTarget,$(strip $(target)))) \
	$(eval $(call defineObject,$(strip $(call targetName,$(target))))))



# +--------------------------------------------------------------------------+
# |   SMART MESSAGE PRINTING                                                 |
# +--------------------------------------------------------------------------+


# Smart messages
cmd_ar_a_o	= AR                $@
cmd_cc_o_c	= CC                $<
cmd_cxx_o_cpp	= CXX               $<
cmd_yacc_h	= YACC [H]          $<
cmd_yacc_cpp	= YACC [CPP]        $<
cmd_lex_cpp	= LEX               $<
cmd_link	= LINK              $@
cmd_ln_so_o	= LINK [M]          $@
cmd_rm_clean	= RM                *.o
cmd_rm_distclean = RM                $(_TARGETS) *.d $(DEFFILE)



# +--------------------------------------------------------------------------+
# |   TARGET RULES                                                           |
# +--------------------------------------------------------------------------+

TARGET_RULES    := a so bundle

# Archives
define targetDefinition_a
$(value $(1)_TARGET): $(value $(1)_OBJ)
	$$(call printCmd, $$(cmd_ar_a_o))
	$$(Q)$$(AR) $$@ $$^
endef
-include rules.mk

# Plugins (for MacOS X)
define targetDefinition_bundle
$(value $(1)_TARGET): $(value $(1)_OBJ) $(value $(1)_LOADER)
	$$(call printCmd, $$(cmd_ln_so_o))
	$$(Q)$$(LINKER) $$(MODULE_FLAGS) $$(LDFLAGS) -o $$@ $$(value $(1)_OBJ) \
		-bundle -bundle_loader $$(value $(1)_LOADER) \
		$$(value $(1)_LIBS) $$(value $(1)_FLAGS) $$(LIBS)
endef

# Plugins and libaries (for UNIXes)
define targetDefinition_so
$(value $(1)_TARGET): $(value $(1)_OBJ)
	$$(call printCmd, $$(cmd_ln_so_o))
	$$(Q)$$(LINKER) $$(MODULE_FLAGS) $$(LDFLAGS) -o $$@ $$(value $(1)_OBJ) \
		-rdynamic -shared $$(value $(1)_LIBS) $$(value $(1)_FLAGS) \
		$$(LIBS)
endef

rulesTarget	:= $(foreach rules,$(TARGET_RULES),$(filter \
			%.$(rules),$(TARGETS)))
$(foreach target,$(rulesTarget),$(eval $(call targetDefinition_$(subst \
	.,,$(suffix $(target))),$(call targetName,$(target)))))



# +--------------------------------------------------------------------------+
# |   COMPILATION RULES                                                      |
# +--------------------------------------------------------------------------+

# C Files
$(BUILDDIR)/%.o: $(BUILDDIR)/%.c
	$(call printCmd, $(cmd_cc_o_c))
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) $(call flags,$<) -o $@ -c $<
$(BUILDDIR)/%.o: %.c
	$(call printCmd, $(cmd_cc_o_c))
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) $(call flags,$<) -o $@ -c $<

# C++ Files
$(BUILDDIR)/%.o: $(BUILDDIR)/%.cpp
	$(call printCmd, $(cmd_cxx_o_cpp))
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) $(call flags,$<) -o $@ -c $<
$(BUILDDIR)/%.o: %.cpp
	$(call printCmd, $(cmd_cxx_o_cpp))
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) $(call flags,$<) -o $@ -c $<

# Yacc compilation
$(BUILDDIR)/%.yy.h: %.y
	$(call printCmd, $(cmd_yacc_h))
	@mkdir -p $(dir $@)
	$(Q)$(YACC) $(call flags,$<) -d -b $(basename $(basename $@)) \
		-p $(basename $(notdir $<)) $<
	$(Q)rm $(basename $(basename $@)).tab.c
	$(Q)mv $(basename $(basename $@)).tab.h $(basename $@).h
$(BUILDDIR)/%.yy.cpp: %.y
	$(call printCmd, $(cmd_yacc_cpp))
	@mkdir -p $(dir $@)
	$(Q)$(YACC) $(call flags,$<) -b $(basename $(basename $@)) \
		-p $(basename $(notdir $<)) -o $@ $<

# Lex compilation
$(BUILDDIR)/%.l.cpp: %.l $(BUILDDIR)/%.yy.h
	$(call printCmd, $(cmd_lex_cpp))
	$(Q)$(LEX) $(call flags,$<) -P$(basename $(notdir $<)) -t $< > $@



# +--------------------------------------------------------------------------+
# |   CLEAN RULES                                                            |
# +--------------------------------------------------------------------------+

.PHONY: clean distclean
clean:
	$(call printCmd, $(cmd_rm_clean))
	$(Q)$(RM) $(foreach target,$(TARGETS),$(value $(call \
		targetName,$(target))_OBJ) $(value $(target)_CLEAN))

distclean: clean
	$(call printCmd, $(cmd_rm_distclean))
	$(Q)$(RM) $(foreach dep,$(DEPENDENCIES),$(DEPDIR)/$(dep))
	$(Q)$(RM) $(_TARGETS)
	$(Q)$(RM) $(DEFFILE)



# +--------------------------------------------------------------------------+
# |   GET ALL SUBDIRS TO EXPLORE                                             |
# +--------------------------------------------------------------------------+

# Generate the defs.mk file which contains sub directories
$(DEFFILE): Makefile $(patsubst %, %/module.mk, $(SUBDIRS)) module.mk
	@echo -n "     GEN               $(DEFFILE)"
	@echo "" > $@
	@make -s -C ./ _depsreload

.PHONY: _depsreload
_depsreload:
	@echo -n "."
	@echo "DEFLOADED := 1" > $(DEFFILE)
	@echo "_SUBDIRS := $(SUBDIRS)" >> $(DEFFILE)
	@if [ "$(SUBDIRS)" != "$(_SUBDIRS)" ]; then make -j 1 -s -C ./ _depsreload; \
		else echo "DEFDONE := 1" >> $(DEFFILE); echo ""; fi

