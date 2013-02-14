include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libsfm.a

EXT_INCL := -I..
EXT_LIBS := -L. -L../mve -L../util -lmve -lutil -lpng -ljpeg -ltiff

TESTSRC := _test_matching.cc
TESTBIN := test

GTEST_SRCS := _gtest_matrixsvd.cc _gtest_matrixpinv.cc _gtest_pose.cc # $(wildcard _gtest_*.cc)
GTEST_OBJS := $(foreach file,$(GTEST_SRCS),$(subst .cc,.o,$(file)))
GTEST_INCL := -I${GTEST_PATH}/include
GTEST_LIBS := -L${GTEST_PATH}/make -lgtest_main

libsfm: ${OBJECTS}
	ar rcs ${LIBRARY} ${OBJECTS}
	chmod a+x ${LIBRARY}

test: libsfm FORCE
	${CXX} -o ${TESTBIN} ${OBJECTS} ${TESTSRC} ${CXXFLAGS} ${EXT_INCL} ${EXT_LIBS}

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
