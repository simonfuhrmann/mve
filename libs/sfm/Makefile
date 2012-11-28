include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libsfm.a
TESTSRC := _test.cc
TESTBIN := test

EXT_INCL := -I..
EXT_LIBS := -L. -L../mve -L../util -lmve -lutil -lpng -ljpeg -ltiff

libsfm: ${OBJECTS}
	ar rcs ${LIBRARY} ${OBJECTS}
	chmod a+x ${LIBRARY}

test: libsfm FORCE
	${CXX} -o ${TESTBIN} ${OBJECTS} ${TESTSRC} ${CXXFLAGS} ${EXT_INCL} ${EXT_LIBS}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

depend:
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${LIBRARY} ${TESTBIN}

FORCE:

-include Makefile.dep
