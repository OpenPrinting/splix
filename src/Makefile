#
# 	Makefile			(C) 2006, Aurélien Croc (AP²C)
#
#  This project has been placed under the GPL Licence.
#

CXXFLAGS	+= -I../include
LDFLAGS		+= -lcups -lcupsimage

OBJECTS		:= spl2.o printer.o band.o compress.o
HEADERS		:= include/spl2.h include/document.h include/printer.h \
		   include/error.h include/band.h include/raster.h


all: rastertospl2
rastertospl2: $(OBJECTS) rastertospl2.o raster.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

install: rastertospl2
	install -m 755 -s rastertospl2 ${CUPSFILTER}

.PHONY: clean distclean
clean:
	$(RM) *.o

distclean: clean
	$(RM) rastertospl2

