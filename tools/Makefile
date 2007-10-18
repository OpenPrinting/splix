#
#   Makefile			(C) 2006-2007, Aurélien Croc (AP²C)
# 

SRCS		:= decompress.cpp appliargs.cpp pjl.cpp qpdl.cpp page.cpp \
		   algo0x11.cpp
HEADERS		:= appliargs.h i18n.h pjl.h qpdl.h page.h algo0x11.h
PROJECT		:= decompress


CXXFLAGS  	:= -O0 -g `pkg-config QtCore --cflags`
LIBS		:= `pkg-config QtCore --libs`

OBJS		:= $(SRCS:.cpp=.o)

all: $(PROJECT)

$(OBJS): $(HEADERS)

$(PROJECT): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)


.PHONY: clean cleanall
clean:
	$(RM) $(OBJS)

cleanall: clean
	$(RM) $(PROJECT)
