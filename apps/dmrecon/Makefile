include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
BINARY := dmrecon
LIBDIR := ../../libs
DESTDIR := ../../bin
EXTLIBS := ../../extern
OPENMP := -fopenmp

EXT_INCL := -I${LIBDIR} -I${EXTLIBS}
EXT_LIBS := -lpng -ljpeg -ltiff

${BINARY}: ${OBJECTS} ${LIBDIR}/dmrecon/libdmrecon.a ${LIBDIR}/mve/libmve.a ${LIBDIR}/util/libutil.a
	${CXX} -o $@ $^ ${EXT_LIBS} ${OPENMP}

# TODO: find a nicer way to handle this
${LIBDIR}/dmrecon/libdmrecon.a:
	${MAKE} -C ${LIBDIR}/dmrecon

.PHONY: ${LIBDIR}/dmrecon/libdmrecon.a

merge: _merge.cc
	${CXX} -o $@ $^ ${EXT_INCL} ${EXT_LIBS}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL} ${OPENMP} -Wno-long-long

Makefile.dep: ${SOURCES}
	${CXX} -MM ${SOURCES} ${EXT_INCL} >$@

clean:
	${RM} ${OBJECTS} ${BINARY} merge Makefile.dep

install: ${BINARY}
	${MKDIR} ${DESTDIR}
	${CP} ${BINARY} ${DESTDIR}

.PHONY: clean install

-include Makefile.dep
