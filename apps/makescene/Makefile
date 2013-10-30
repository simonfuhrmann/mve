MVE_ROOT := ../..
TARGET := $(shell basename `pwd`)
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I${MVE_ROOT}/libs -fopenmp
LDLIBS += -lpng -ltiff -ljpeg -fopenmp

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o} libmve.a libmve_util.a

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
