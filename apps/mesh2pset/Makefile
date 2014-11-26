MVE_ROOT := ../../../mve
TARGET := $(shell basename `pwd`)
include ${MVE_ROOT}/Makefile.inc

FSSR_ROOT := ../..
vpath libfssr.a ${FSSR_ROOT}/libs/fssr/

CXXFLAGS += -I${FSSR_ROOT}/libs -I${MVE_ROOT}/libs -fopenmp
LDLIBS += -lpng -ltiff -ljpeg -fopenmp

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o} libfssr.a libmve.a libmve_util.a

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
