include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libdmrecon.a

EXT_INCL := -I..
EXT_LIBS := -L../util -L../mve -lmve -lutil -lpng -ljpeg -ltiff

${LIBRARY}: ${OBJECTS}
	ar rcs $@ $^
	chmod a+x $^

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

Makefile.dep: ${SOURCES}
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${LIBRARY} ${TESTBIN}

FORCE:

-include Makefile.dep
