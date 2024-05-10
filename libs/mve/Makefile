MVE_ROOT := ../..
TARGET := libmve.a
include ${MVE_ROOT}/Makefile.inc

# Position independent code (-fPIC) is required for the UMVE plugin system.
CXXFLAGS += -fPIC -I${MVE_ROOT}/libs ${LIBJPEG_CFLAGS} ${LIBPNG_CFLAGS} ${LIBTIFF_CFLAGS}
LDLIBS += ${LIBJPEG_LDFLAGS} ${LIBPNG_LDFLAGS} ${LIBTIFF_LDFLAGS}

SOURCES := $(wildcard [^_]*.cc)
${TARGET}: ${SOURCES:.cc=.o}
	$(AR) rcs $@ $^

_test%: _test%.o libmve.a libmve_util.a
	${LINK.cc} -o $@ $^ ${LDLIBS}

clean:
	${RM} ${TARGET} *.o Makefile.dep

.PHONY: clean
