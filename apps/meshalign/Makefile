MVE_ROOT := ../../../mve
TARGET := $(shell basename `pwd`)
include ${MVE_ROOT}/Makefile.inc

MSPR_ROOT := ../..
vpath libfssr.a ${MSPR_ROOT}/libs/fssr/

CXXFLAGS += -I${MSPR_ROOT}/libs -I${MVE_ROOT}/libs -fopenmp
LDLIBS += -lpng -ltiff -ljpeg -fopenmp

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o} libmve.a libmve_util.a
	$(LINK.o) $^ $(LDLIBS) -o $@

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
