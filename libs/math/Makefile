include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libmath.a
TESTSRC := _test.cc
TESTBIN := test

EXT_INCL := -I..
EXT_LIBS := -lpthread

libutil: ${OBJECTS}
	#ar rcs ${LIBRARY} ${OBJECTS}
	#chmod a+x ${LIBRARY}

test: ${OBJECTS} FORCE
	${CXX} -o ${TESTBIN} ${TESTSRC} ${OBJECTS} ${EXT_INCL} ${EXT_LIBS} ${CXXFLAGS}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

depend:
	# This is commented out because there are no .cc files
	#${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${LIBRARY} ${TESTBIN}

FORCE:

-include Makefile.dep
