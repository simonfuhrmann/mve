include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
BINARY := dmrecon
LIBDIR := ../../libs
EXTLIBS := ../../extern
OPENMP := -fopenmp

EXT_INCL := -I${LIBDIR} -I${EXTLIBS}
EXT_LIBS := -L${LIBDIR}/util -L${LIBDIR}/dmrecon -L${LIBDIR}/mve -ldmrecon -lmve -lutil -lpng -ljpeg -ltiff

all: libdmrecon ${OBJECTS}
	${CXX} -o ${BINARY} ${OBJECTS} ${EXT_LIBS} ${OPENMP}

libsyndmr:
	${MAKE} -j${CORES} -C ../../libs/syndmr

libdmrecon:
	${MAKE} -j${CORES} -C ../../libs/dmrecon

libclean:
	${MAKE} -C ../../libs/dmrecon clean

libs:
	${MAKE} -C ../../libs

dmsr: FORCE
	${CXX} -o dmsr _dmsr.cc ${EXT_INCL} -L${LIBDIR}/util -L${LIBDIR}/dmrecon -L${LIBDIR}/mve -ldmrecon -lmve -lutil -lpng -ljpeg -ltiff

merge: FORCE
	${CXX} -o merge _merge.cc ${EXT_INCL} ${EXT_LIBS}

saveImages: FORCE
	${CXX} -o saveImages _saveImages.cc ${EXT_INCL} ${EXT_LIBS}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL} ${OPENMP} -Wno-long-long

depend:
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${BINARY} merge

FORCE:

-include Makefile.dep
