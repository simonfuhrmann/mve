include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libmve.a

EXT_INCL := -I..
EXT_LIBS := -lpng -ljpeg -ltiff

TEST_SRC := _test_image.cc
TEST_BIN := _test

GTEST_SRCS := $(wildcard _gtest_*.cc)
GTEST_OBJS := $(foreach file,$(GTEST_SRCS),$(subst .cc,.o,$(file)))
GTEST_INCL := -I${GTEST_PATH}/include
GTEST_LIBS := -L${GTEST_PATH}/make -lgtest_main -lpthread

${LIBRARY}: ${OBJECTS}
	ar rcs $@ $^
	chmod a+x $@

${TEST_BIN}: ${TEST_SRC} ${LIBRARY} ../util/libutil.a
	${CXX} -o $@ $^ ${CXXFLAGS} ${EXT_INCL} ${EXT_LIBS}

_gtest: ${GTEST_OBJS} ${LIBRARY} ../util/libutil.a
	${CXX} -o $@ $^ ${GTEST_LIBS} ${EXT_LIBS}

_gtest_%.o: _gtest_%.cc
	${CXX} -c -o $@ $< ${GTEST_INCL} ${EXT_INCL}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

Makefile.dep: ${SOURCES}
	${CXX} -MM ${SOURCES} ${EXT_INCL} >$@

clean:
	${RM} ${OBJECTS} ${LIBRARY} ${TEST_BIN} _gtest ${GTEST_OBJS} Makefile.dep

.PHONY: clean

-include Makefile.dep
