MVE_ROOT := ..
TARGET := test
include ${MVE_ROOT}/Makefile.inc
vpath gtest_main.a ${GTEST_PATH}/make/

SOURCES = $(wildcard math/gtest_*.cc) $(wildcard mve/gtest_*.cc) $(wildcard sfm/gtest_*.cc) $(wildcard util/gtest_*.cc)
CXXFLAGS = -g -O3 -pthread -I${MVE_ROOT}/libs -I${GTEST_PATH}/include
LDLIBS += -lpng -ltiff -ljpeg

test: ${SOURCES:.cc=.o} gtest_main.a libmve_sfm.a libmve.a libmve_util.a
	${LINK.cc} -o $@ $^ ${LDLIBS}

clean:
	${RM} ${TARGET} mve/*.o util/*.o math/*.o Makefile.dep

.PHONY: test
