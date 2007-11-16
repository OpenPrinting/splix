#
#   Makefile			(C) 2006-2007, Aurélien Croc (AP²C)
# 

SRCS_decompress	:= decompress.cpp appliargs.cpp pjl.cpp qpdl.cpp page.cpp \
		   algo0x11.cpp
HEADERS_decompress:= appliargs.h i18n.h pjl.h qpdl.h page.h algo0x11.h

SRCS_jbgtopbm	:= jbgtopbm.cpp appliargs.cpp
HEADERS_jbgtopbm:= appliargs.h

PROJECTS	:= decompress jbgtopbm

CXXFLAGS  	:= -O0 -g `pkg-config QtCore --cflags`
LIBS		:= `pkg-config QtCore --libs`

OBJS_decompress	:= $(SRCS_decompress:.cpp=.o)
OBJS_jbgtopbm	:= $(SRCS_jbgtopbm:.cpp=.o)

all: $(PROJECTS)

$(OBJS_decompress): $(HEADERS_decompress)
$(OBJS_jbgtopbm): $(HEADERS_jbgtopbm)

decompress: $(OBJS_decompress)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

jbgtopbm: $(OBJS_jbgtopbm)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS) -ljbig

.PHONY: clean cleanall
clean:
	$(RM) $(OBJS_decompress) $(OBJS_jbgtopbm)

cleanall: clean
	$(RM) $(PROJECTS)
