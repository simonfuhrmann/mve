include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
BINARY := bundle2ply
LIBDIR := ../../libs
DESTDIR := ../../bin

EXT_INCL := -I../../libs
EXT_LIBS := -lpng -ljpeg -ltiff

${BINARY}: ${OBJECTS} ${LIBDIR}/mve/libmve.a ${LIBDIR}/util/libutil.a
	${CXX} -o $@ $^ ${EXT_LIBS}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

Makefile.dep: ${SOURCES}
	${CXX} -MM ${SOURCES} ${EXT_INCL} >$@

clean:
	${RM} ${OBJECTS} ${BINARY} ${TESTBIN} Makefile.dep

install: ${BINARY}
	${MKDIR} ${DESTDIR}
	${CP} ${BINARY} ${DESTDIR}

.PHONY: clean install

-include Makefile.dep
