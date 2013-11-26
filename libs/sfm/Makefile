MVE_ROOT := ../..
TARGET := libmve_sfm.a
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I${MVE_ROOT}/libs
LDLIBS += -lpng -ltiff -ljpeg

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o}
	$(AR) rcs $@ $^

_test%: _test%.o libmve.a libmve_util.a
	${LINK.cc} -o $@ $^ ${LDLIBS}

_gtest: CXXFLAGS += -I ${GTEST_PATH}/include
_gtest: _gtest_*.cc ${TARGET} libmve.a libmve_util.a libgtest_main.a
	${LINK.cc} -o $@ $^ ${LDLIBS}

clean:
	${RM} ${TARGET} *.o Makefile.dep _gtest

.PHONY: clean
