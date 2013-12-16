MVE_ROOT ?= ../../../../mve
TARGET := libdmf.a
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I${MVE_ROOT}/libs -fopenmp

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o}
	ar rcs $@ $^
	chmod a+x $@

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
