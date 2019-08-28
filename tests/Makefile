MVE_ROOT := ..
TARGET := test
include ${MVE_ROOT}/Makefile.inc
vpath libgtest_main.a ${GTEST_PATH}/build/lib
vpath libgtest.a ${GTEST_PATH}/build/lib

SOURCES = $(wildcard math/gtest_*.cc) $(wildcard mve/gtest_*.cc) $(wildcard sfm/gtest_*.cc) $(wildcard util/gtest_*.cc) $(wildcard fssr/gtest_*.cc)
INCLUDES = -I${MVE_ROOT}/libs -I${GTEST_PATH}/include
CXXWARNINGS = -Wall -Wextra -pedantic -Wno-sign-compare
CXXFLAGS = -std=c++11 -pthread ${CXXWARNINGS} ${INCLUDES}
LDLIBS += -lpng -ltiff -ljpeg

test: ${SOURCES:.cc=.o} libmve_fssr.a libmve_sfm.a libmve.a libmve_util.a libgtest_main.a libgtest.a
	${LINK.cc} -o $@ $^ ${LDLIBS}

clean:
	${RM} ${TARGET} mve/*.o util/*.o math/*.o sfm/*.o fssr/*.o Makefile.dep

.PHONY: test
