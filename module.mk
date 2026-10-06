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
# 	   DRV_ONLY     Don't install PPD files at all, only DRV files.

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
DRV_ONLY		?= 0


# Flags
ifeq ($(ARCHI),Darwin)
# macOS ships no cups.pc, and the raster API lives in libcups itself.
# jbigkit is searched in Homebrew (Apple Silicon, Intel) and MacPorts.
# Set ARCHFLAGS="-arch arm64 -arch x86_64" for a universal binary.
JBIGDIR			?= $(firstword $(wildcard /opt/homebrew/opt/jbigkit /usr/local/opt/jbigkit /opt/local))
CXXFLAGS		+= $(ARCHFLAGS) `cups-config --cflags` -Iinclude -Wall -I$(JBIGDIR)/include
LDFLAGS			+= $(ARCHFLAGS) -L$(JBIGDIR)/lib
CUPSLIBS		:= `cups-config --libs`
else
CXXFLAGS		+= `pkg-config --cflags cups` -Iinclude -Wall -I/opt/local/include
CUPSLIBS		:= `pkg-config --libs cups` -lcupsimage
endif
DEBUG_CXXFLAGS		+= -DDEBUG  -DDUMP_CACHE
OPTIM_CXXFLAGS 		+= -g
rastertoqpdl_LDFLAGS	:= $(LDFLAGS) -L/opt/local/lib
rastertoqpdl_LIBS	:= $(CUPSLIBS)
pstoqpdl_LDFLAGS	:= $(LDFLAGS)
pstoqpdl_LIBS		:= $(CUPSLIBS)


# Update compilation flags with defined options
ifneq ($(DISABLE_THREADS),0)
CXXFLAGS		+= -DDISABLE_THREADS
else
CXXFLAGS		+= -DTHREADS=$(THREADS) -DCACHESIZE=$(CACHESIZE)
rastertoqpdl_LIBS	+= -lpthread
pstoqpdl_LIBS		+= -lpthread
endif
ifneq ($(DISABLE_JBIG),0)
CXXFLAGS		+= -DDISABLE_JBIG
else
rastertoqpdl_LIBS	+= -ljbig85
endif
ifneq ($(DISABLE_BLACKOPTIM),0)
CXXFLAGS		+= -DDISABLE_BLACKOPTIM
endif


# Get some information
ifeq ($(ARCHI),Darwin)
CUPSFILTER		:= `cups-config --serverbin`/filter
CUPSPPD			?= `cups-config --datadir`/model
CUPSDRV			?= `cups-config --datadir`/drv
CUPSPROFILE		:= `cups-config --datadir`/profiles
else
CUPSFILTER		:= `pkg-config --variable=cups_serverbin cups`/filter
CUPSPPD			?= `pkg-config --variable=cups_datadir cups`/model
CUPSDRV			?= `pkg-config --variable=cups_datadir cups`/drv
CUPSPROFILE		:= `pkg-config --variable=cups_datadir cups`/profiles
endif
ifeq ($(ARCHI),Darwin)
PSTORASTER		:= pstocupsraster
else
PSTORASTER		:= pstoraster
endif
GSTORASTER		:= gstoraster
export CUPSFILTER CUPSPPD CUPSDRV


# Specific information needed by pstoqpdl
src_pstoqpdl_cpp_FLAGS	:= -DRASTERDIR=\"$(CUPSFILTER)\"
src_pstoqpdl_cpp_FLAGS	+= -DRASTERTOQPDL=\"rastertoqpdl\"
src_pstoqpdl_cpp_FLAGS	+= -DPSTORASTER=\"$(PSTORASTER)\"
src_pstoqpdl_cpp_FLAGS	+= -DGSTORASTER=\"$(GSTORASTER)\"
src_pstoqpdl_cpp_FLAGS	+= -DCUPSPPD=\"$(CUPSPPD)\"
src_pstoqpdl_cpp_FLAGS	+= -DCUPSPROFILE=\"$(CUPSPROFILE)\"

