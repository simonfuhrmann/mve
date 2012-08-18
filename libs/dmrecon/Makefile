include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cpp)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cpp,.o,$(file)))
LIBRARY := libdmrecon.a
TESTSRC := _test.cc
TESTBIN := test

EXT_INCL := -I..
EXT_LIBS := -L../util -L../mve -lmve -lutil -lpng -ljpeg -ltiff

libmve: ${OBJECTS}
	ar rcs ${LIBRARY} ${OBJECTS}
	chmod a+x ${LIBRARY}

test: libdmrecon FORCE
	${CXX} -o ${TESTBIN} ${TESTSRC} ${CXXFLAGS} ${EXT_INCL} ${EXT_LIBS}

%.o: %.cpp
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

depend:
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${LIBRARY} ${TESTBIN}

FORCE:

-include Makefile.dep
