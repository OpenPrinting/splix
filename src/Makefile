#
# 	Makefile			(C) 2006, Aurélien Croc (AP²C)
#
#  This project has been placed under the GPL Licence.
#

CXXFLAGS	+= -I../include -Wall -g -O0 #-DENABLE_DEBUG
LDFLAGS		+=
LIBS		+= -lcups -lcupsimage -ljbig

OBJECTS		:= spl2.o printer.o band.o compress.o bandanalyser.o
HEADERS		:= include/spl2.h include/document.h include/printer.h \
		   include/error.h include/band.h include/raster.h \
		   include/bandanalyser.h


all: rastertospl2
rastertospl2: $(OBJECTS) rastertospl2.o raster.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

pbmtospl2: $(OBJECTS) pbmtospl2.o pbmimage.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

install: rastertospl2
	install -m 755 -s rastertospl2 ${CUPSFILTER}

.PHONY: clean distclean
clean:
	$(RM) *.o

distclean: clean
	$(RM) rastertospl2 pbmtospl2

