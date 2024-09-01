MVE_ROOT := ../..
TARGET := libmve_fssr.a
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -fPIC -I${MVE_ROOT}/libs ${OPENMP}
LDLIBS += ${LIBJPEG_LDFLAGS} ${LIBPNG_LDFLAGS} ${LIBTIFF_LDFLAGS} ${OPENMP}

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o}
	$(AR) rcs $@ $^

clean:
	${RM} $(TARGET) *.o Makefile.dep

.PHONY: clean
