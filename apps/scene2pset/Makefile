include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
BINARY := scene2pset
LIBDIR := ../../libs
DESTDIR := ../../bin
OPENMP := -fopenmp

EXT_INCL := -I${LIBDIR}
EXT_LIBS := -lpng -ljpeg -ltiff

${BINARY}: ${OBJECTS} ${LIBDIR}/mve/libmve.a ${LIBDIR}/util/libutil.a
	${CXX} -o $@ $^ ${EXT_LIBS} ${OPENMP}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL} ${OPENMP}

Makefile.dep: ${SOURCES}
	${CXX} -MM ${SOURCES} ${EXT_INCL} >$@

clean:
	${RM} ${OBJECTS} ${BINARY} ${TESTBIN} Makefile.dep

install: ${BINARY}
	${MKDIR} ${DESTDIR}
	${CP} ${BINARY} ${DESTDIR}

.PHONY: clean install

-include Makefile.dep
