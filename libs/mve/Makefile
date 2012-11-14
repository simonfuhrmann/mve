include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libmve.a
TESTSRC := _test_image.cc
TESTBIN := test

EXT_INCL := -I..
EXT_LIBS := -L. -L../util -lmve -lutil -lpng -ljpeg -ltiff

libmve: ${OBJECTS}
	ar rcs ${LIBRARY} ${OBJECTS}
	chmod a+x ${LIBRARY}

test: libmve FORCE
	${CXX} -o ${TESTBIN} ${TESTSRC} ${CXXFLAGS} ${EXT_INCL} ${EXT_LIBS}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

depend:
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${LIBRARY} ${TESTBIN}

FORCE:

-include Makefile.dep
