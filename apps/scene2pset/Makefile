include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
BINARY := scene2pset
LIBDIR := ../../libs
OPENMP := -fopenmp

EXT_INCL := -I${LIBDIR}
EXT_LIBS := -L${LIBDIR}/util -L${LIBDIR}/mve -lmve -lutil -lpng -ljpeg -ltiff

all: ${OBJECTS}
	${CXX} -o ${BINARY} ${OBJECTS} ${EXT_LIBS} ${OPENMP}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL} ${OPENMP}

depend:
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${BINARY} ${TESTBIN}

FORCE:

-include Makefile.dep
