#
#	module.mk			(C) 2007-2008, Aurélien Croc (AP²C)
#
#  Compilation file for SpliX
#
# Options: DISABLE_JBIG
# 	   DISABLE_THREADS
#          DISABLE_BLACKOPTIM
# Compilation option:
# 	   V=1          Verbose mode
# 	   DESTDIR=xxx  Change the destination directory prefix

MODE			:= optimized

SUBDIRS 		+= src
TARGETS			:= rastertoqpdl pstoqpdl
PRE_GENERIC_TARGETS	:= optionList


# Default options
THREADS			?= 2
CACHESIZE		?= 30
DISABLE_JBIG		?= 0
DISABLE_THREADS		?= 0
DISABLE_BLACKOPTIM	?= 0


# Flags
CXXFLAGS		+= `cups-config --cflags` -Iinclude -Wall -I/opt/local/include
DEBUG_CXXFLAGS		+= -DDEBUG  -DDUMP_CACHE
OPTIM_CXXFLAGS 		+= -g
rastertoqpdl_LDFLAGS	:= `cups-config --ldflags` -L/opt/local/lib
rastertoqpdl_LIBS	:= `cups-config --libs` -lcupsimage
pstoqpdl_LDFLAGS	:= `cups-config --ldflags`
pstoqpdl_LIBS		:= `cups-config --libs` -lcupsimage


# Update compilation flags with defined options
ifneq ($(DISABLE_THREADS),0)
CXXFLAGS		+= -DDISABLE_THREADS
else
CXXFLAGS		+= -DTHREADS=$(THREADS) -DCACHESIZE=$(CACHESIZE)
endif
ifneq ($(DISABLE_JBIG),0)
CXXFLAGS		+= -DDISABLE_JBIG
else
rastertoqpdl_LIBS	+= -ljbig
endif
ifneq ($(DISABLE_BLACKOPTIM),0)
CXXFLAGS		+= -DDISABLE_BLACKOPTIM
endif


# Get some information
CUPSFILTER		:= `cups-config --serverbin`/filter
CUPSPPD			:= `cups-config --datadir`/model
CMSBASE			:= $(CUPSPPD)/samsung/cms
ifeq ($(ARCHI),Darwin)
PSTORASTER		:= pstocupsraster
else
PSTORASTER		:= pstoraster
endif
export CUPSFILTER CUPSPPD


# Specific information needed by pstoqpdl
src_pstoqpdl_cpp_FLAGS	:= -DRASTERDIR=\"$(CUPSFILTER)\"
src_pstoqpdl_cpp_FLAGS	+= -DRASTERTOQPDL=\"rastertoqpdl\"
src_pstoqpdl_cpp_FLAGS	+= -DPSTORASTER=\"$(PSTORASTER)\"
src_pstoqpdl_cpp_FLAGS	+= -DCMSBASE=\"$(CMSBASE)\"

