MVE_ROOT := ../../../mve
TARGET := $(shell basename `pwd`)
include ${MVE_ROOT}/Makefile.inc

MSPR_ROOT := ../..
vpath libfssr.a ${MSPR_ROOT}/libs/fssr/

CXXFLAGS += -I${MSPR_ROOT}/libs -I${MVE_ROOT}/libs ${OPENMP}
LDLIBS += -lpng -ltiff -ljpeg ${OPENMP}

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o} libfssr.a libmve.a libmve_util.a

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
