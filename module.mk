#
#	module.mk			(C) 2007-2008, Aurélien Croc (AP²C)
#
#  Compilation file for SpliX
#
# Options: DISABLE_JBIG
# 	   DISABLE_THREADS
#          DISABLE_BLACKOPTIM

MODE			:= debug

SUBDIRS 		+= src
TARGETS			:= rastertoqpdl pstoqpdl


# Flags
CXXFLAGS		+= `cups-config --cflags` -Iinclude -Wall
CXXFLAGS		+= -DTHREADS=2 -DCACHESIZE=2
DEBUG_CXXFLAGS		+= -DDEBUG  -DDUMP_CACHE
OPTIMIZED_CXXFLAGS 	+= -g
OPTIMIZED_CXXFLAGS 	+= -g 
rastertoqpdl_LDFLAGS	:= `cups-config --ldflags`
rastertoqpdl_LIBS	:= `cups-config --libs` -lcupsimage
pstoqpdl_LDFLAGS	:= `cups-config --ldflags`
pstoqpdl_LIBS		:= `cups-config --libs` -lcupsimage
ifndef $(DISABLE_JBIG)
rastertoqpdl_LIBS	+= -ljbig
endif


# Get some information
CMSBASE			:= `cups-config --datadir`/model/samsung/cms
CUPSFILTER		:= `cups-config --serverbin`/filter
ifeq ($(ARCHI),Darwin)
PSTORASTER		:= pstocupsraster
else
PSTORASTER		:= pstoraster
endif


# Specific information needed by pstoqpdl
src_pstoqpdl_cpp_FLAGS	:= -DRASTERDIR=\"$(CUPSFILTER)\"
src_pstoqpdl_cpp_FLAGS	+= -DRASTERTOQPDL=\"rastertoqpdl\"
src_pstoqpdl_cpp_FLAGS	+= -DPSTORASTER=\"$(PSTORASTER)\"
src_pstoqpdl_cpp_FLAGS	+= -DCMSBASE=\"$(CMSBASE)\"

