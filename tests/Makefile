MVE_ROOT := ..
TARGET := test
include ${MVE_ROOT}/Makefile.inc

GTEST_CFLAGS = `pkg-config --cflags gtest_main`
GTEST_LDFLAGS = `pkg-config --libs gtest_main`

SOURCES = $(wildcard math/gtest_*.cc) $(wildcard mve/gtest_*.cc) $(wildcard sfm/gtest_*.cc) $(wildcard util/gtest_*.cc) $(wildcard fssr/gtest_*.cc)
INCLUDES = -I${MVE_ROOT}/libs ${GTEST_CFLAGS}
CXXWARNINGS = -Wall -Wextra -pedantic -Wno-sign-compare
CXXFLAGS = -std=c++17 -pthread ${CXXWARNINGS} ${INCLUDES}
LDLIBS += ${GTEST_LDFLAGS} ${LIBJPEG_LDFLAGS} ${LIBPNG_LDFLAGS} ${LIBTIFF_LDFLAGS}

test: ${SOURCES:.cc=.o} libmve_fssr.a libmve_sfm.a libmve.a libmve_util.a
	${LINK.cc} -o $@ $^ ${LDLIBS}

clean:
	${RM} ${TARGET} mve/*.o util/*.o math/*.o sfm/*.o fssr/*.o Makefile.dep

.PHONY: test
