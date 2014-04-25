MVE_ROOT := ../..
TARGET := libmve_math.a
include ${MVE_ROOT}/Makefile.inc

# Position independent code (-fPIC) is required for the UMVE plugin system.
CXXFLAGS += -fPIC -I${MVE_ROOT}/libs

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o}
	#$(AR) rcs $@ $^

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
