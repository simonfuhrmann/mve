include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libutil.a
TESTBIN := test
TESTSRC := _test.cc

EXT_INCL := -I..
EXT_LIBS := -lpthread

GTEST_INCL := -I/Users/simon/Downloads/gtest-1.6.0/include
GTEST_LIBS := -L/Users/simon/Downloads/gtest-1.6.0/make -lgtest_main

libutil: ${OBJECTS}
	ar rcs ${LIBRARY} ${OBJECTS}
	chmod a+x ${LIBRARY}

gtest:
	${CXX} _test_refptr.cc -o _test_refptr ${GTEST_INCL} ${GTEST_LIBS}

test: ${OBJECTS} FORCE
	${CXX} ${TESTSRC} -o ${TESTBIN} ${OBJECTS} ${EXT_INCL} ${EXT_LIBS}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

depend:
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${LIBRARY} ${TESTBIN}

FORCE:

-include Makefile.dep
