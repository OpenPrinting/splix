#
#   Part of the SpliX project          (C) 2006-2008 by Aurélien Croc (AP²C)
#

rastertoqpdl_SRC	+= src/rastertoqpdl.cpp src/request.cpp \
			   src/printer.cpp src/qpdl.cpp src/document.cpp \
			   src/core.cpp src/compress.cpp src/algorithm.cpp \
			   src/ppdfile.cpp src/page.cpp src/colors.cpp \
			   src/band.cpp src/bandplane.cpp src/algo0x11.cpp \
			   src/cache.cpp src/rendering.cpp src/semaphore.cpp \
			   src/algo0x13.cpp src/algo0x0d.cpp

pstoqpdl_SRC		+= src/pstoqpdl.cpp src/ppdfile.cpp
