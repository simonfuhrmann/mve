MVE_ROOT := ../..
TARGET := $(shell basename `pwd`)
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I${MVE_ROOT}/libs ${OPENMP}
LDLIBS += ${LIBJPEG_LDFLAGS} ${LIBPNG_LDFLAGS} ${LIBTIFF_LDFLAGS} ${OPENMP}

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o} libmve_sfm.a libmve.a libmve_util.a
	$(LINK.o) $^ $(LDLIBS) -o $@

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
