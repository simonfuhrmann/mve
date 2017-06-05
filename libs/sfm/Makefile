MVE_ROOT := ../..
TARGET := libmve_sfm.a
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I${MVE_ROOT}/libs ${OPENMP}
LDLIBS += -lpng -ltiff -ljpeg

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o}
	$(AR) rcs $@ $^

_test%: _test%.o libmve_sfm.a libmve.a libmve_util.a
	${LINK.cc} -o $@ $^ ${LDLIBS}

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
