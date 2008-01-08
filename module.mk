#
#	module.mk			(C) 2007, Aurélien Croc (AP²C)
#
#  Compilation file for SpliX
#

MODE		:= debug

SUBDIRS 	+= src
TARGETS		:= rastertoqpdl
CXXFLAGS	+= -Iinclude -Wall
DEBUG_CXXFLAGS	:= -DDEBUG
rastertoqpdl_LIBS	:= -lcups

# Raster

$(rastertoqpdl_TARGET): $(rastertoqpdl_OBJ)
	$(call printCmd, $(cmd_link))
	$(Q)g++ -o $@ $^ $(rastertoqpdl_CXXFLAGS) $(rastertoqpdl_LDFLAGS) \
		$(rastertoqpdl_LIBS)
