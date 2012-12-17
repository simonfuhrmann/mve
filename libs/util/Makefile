include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libutil.a
TESTBIN := test
TESTSRC := _test.cc

EXT_INCL := -I..
EXT_LIBS := -lpthread

GTEST_SRCS := $(wildcard _gtest_*.cc)
GTEST_OBJS := $(foreach file,$(GTEST_SRCS),$(subst .cc,.o,$(file)))
GTEST_INCL := -I${GTEST_PATH}/include
GTEST_LIBS := -L${GTEST_PATH}/make -lgtest_main

libutil: ${OBJECTS}
	ar rcs ${LIBRARY} ${OBJECTS}
	chmod a+x ${LIBRARY}

test: ${OBJECTS} FORCE
	${CXX} ${TESTSRC} -o ${TESTBIN} ${OBJECTS} ${EXT_INCL} ${EXT_LIBS}

gtest: ${GTEST_OBJS}
	${CXX} ${GTEST_OBJS} ${OBJECTS} -o _gtest ${GTEST_LIBS} ${EXT_LIBS}

_gtest_%.o: _gtest_%.cc
	${CXX} -c -o $@ $< ${GTEST_INCL} ${EXT_INCL}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

depend:
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${LIBRARY} ${TESTBIN}

FORCE:

-include Makefile.dep
