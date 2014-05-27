MVE_ROOT ?= ../../../mve
TARGET := libfssr.a
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I.. -I${MVE_ROOT}/libs ${OPENMP}
LDLIBS += -lpng -ltiff -ljpeg ${OPENMP}

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o}
	$(AR) rcs $@ $^

clean:
	${RM} $(TARGET) *.o Makefile.dep

.PHONY: clean
