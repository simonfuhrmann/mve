include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libmath.a

EXT_INCL := -I..
EXT_LIBS := -lpthread

GTEST_SRCS := $(wildcard _gtest_*.cc)
GTEST_OBJS := $(foreach file,$(GTEST_SRCS),$(subst .cc,.o,$(file)))
GTEST_INCL := -I${GTEST_PATH}/include
GTEST_LIBS := -L${GTEST_PATH}/make -lgtest_main

${LIBRARY}: ${OBJECTS}

_gtest: ${GTEST_OBJS} ${OBJECTS}
	${CXX} -o $@ $^ ${GTEST_LIBS} ${EXT_LIBS}

_gtest_%.o: _gtest_%.cc
	${CXX} -c -o $@ $< ${GTEST_INCL} ${EXT_INCL}

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

Makefile.dep: ${SOURCES}

clean:
	${RM} ${LIBRARY} ${OBJECTS} gtest ${GTEST_OBJS} Makefile.dep

.PHONY: clean

-include Makefile.dep
