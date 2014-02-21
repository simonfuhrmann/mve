MVE_ROOT := ../..
TARGET := libmve_math.a
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I${MVE_ROOT}/libs

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o}
	#$(AR) rcs $@ $^

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
