MVE_ROOT := ../../mve
TARGET := test
include ${MVE_ROOT}/Makefile.inc
vpath gtest_main.a ${GTEST_PATH}/make/

SOURCES = $(wildcard gtest_*.cc)
CXXFLAGS = -g -O3 -I../libs -I${MVE_ROOT}/libs -I${GTEST_PATH}/include
LDLIBS += -lpng -ltiff -ljpeg

vpath libfssr.a ../libs/fssr/
test: ${SOURCES:.cc=.o} gtest_main.a libmve.a libmve_util.a libfssr.a
	${LINK.cc} -o $@ $^ ${LDLIBS}

clean:
	${MAKE} -C ../libs clean
	${RM} ${TARGET} *.o Makefile.dep
