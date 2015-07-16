MVE_ROOT := ../../../mve
TARGET := $(shell basename `pwd`)
include ${MVE_ROOT}/Makefile.inc

FSSR_ROOT := ../..
vpath libfssr.a ${FSSR_ROOT}/libs/fssr/

CXXFLAGS += -I${FSSR_ROOT}/libs -I${MVE_ROOT}/libs ${OPENMP}
LDLIBS += -lpng -ltiff -ljpeg ${OPENMP}

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o} libmve.a libmve_util.a
	$(LINK.o) $^ $(LDLIBS) -o $@

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
