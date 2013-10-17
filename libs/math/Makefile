MVE_ROOT := ../..
TARGET := libmve_math.a
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I${MVE_ROOT}/libs

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o}
	$(AR) rcs $@ $^

_gtest: CXXFLAGS += -I ${GTEST_PATH}/include
_gtest: _gtest_*.cc ${TARGET} libgtest_main.a
	${LINK.cc} -o $@ $^ ${LDLIBS}

clean:
	${RM} ${TARGET} *.o Makefile.dep _gtest

.PHONY: clean
