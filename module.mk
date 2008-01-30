#
#	module.mk			(C) 2007-2008, Aurélien Croc (AP²C)
#
#  Compilation file for SpliX
#
# Options: DISABLE_JBIG
#          DISABLE_BLACKOPTIM

MODE		:= debug

SUBDIRS 	+= src
TARGETS		:= rastertoqpdl
CXXFLAGS	+= `cups-config --cflags` -Iinclude -Wall
DEBUG_CXXFLAGS	+= -DDEBUG
rastertoqpdl_LDFLAGS	:= `cups-config --ldflags`
rastertoqpdl_LIBS	:= `cups-config --libs` -lcupsimage

# Raster

$(rastertoqpdl_TARGET): $(rastertoqpdl_OBJ)
	$(call printCmd, $(cmd_link))
	$(Q)g++ -o $@ $^ $(rastertoqpdl_CXXFLAGS) $(rastertoqpdl_LDFLAGS) \
		$(rastertoqpdl_LIBS)

.PHONY: tags
tags:
	ctags --recurse --language-force=c++ --extra=+q --fields=+i *
