include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
BINARY := mveshell
LIBDIR := ../../libs

EXT_INCL := -I${LIBDIR}
EXT_LIBS := -L${LIBDIR}/util -L${LIBDIR}/mve -lmve -lutil -lpng -ljpeg -ltiff -lreadline

all: ${OBJECTS}
	${CXX} -o ${BINARY} ${OBJECTS} ${EXT_LIBS}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

depend:
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${BINARY}

FORCE:

-include Makefile.dep
