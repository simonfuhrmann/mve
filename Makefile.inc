# Default values.
CXX ?= g++
CXXWARNINGS ?= -Wall -Wextra -Wundef -pedantic
CXXINTRINSICS ?= -msse2 -msse3 -msse4 -mpopcnt
CXXFEATURES ?= -funsafe-math-optimizations -fno-math-errno -std=c++11
CXXFLAGS ?= ${CXXWARNINGS} ${CXXINTRINSICS} ${CXXFEATURES} -g -O3

# Delete this if OpenMP is not available (e.g., OS X without gcc)
OPENMP ?= -fopenmp

# Always compile programs and libs with multi-threading.
CXXFLAGS += -pthread

UNAME = $(shell uname)
ifeq (${UNAME},Darwin)
    OPENMP =
endif

COMPILE.cc = ${CXX} ${CXXFLAGS} ${CPPFLAGS} -c
LINK.o = ${CXX} ${LDFLAGS}
.DEFAULT_GOAL := ${TARGET}

# How to link example from example.o and other files.
%: %.o
	${LINK.o} -o $@ $^ ${LDLIBS}

# How to compile example.o from example.cc.
%.o: %.cc
	${COMPILE.cc} -o $@ $<

# Library paths.
vpath libmve_dmrecon.a ${MVE_ROOT}/libs/dmrecon/
vpath libmve_math.a ${MVE_ROOT}/libs/math/
vpath libmve.a ${MVE_ROOT}/libs/mve/
vpath libmve_ogl.a ${MVE_ROOT}/libs/ogl/
vpath libmve_util.a ${MVE_ROOT}/libs/util/
vpath libmve_sfm.a ${MVE_ROOT}/libs/sfm/
vpath libmve_fssr.a ${MVE_ROOT}/libs/fssr/

# Autogenerate dependencies.
Makefile.dep: [^_]*.cc
	${COMPILE.cc} -MM $^ >$@

-include Makefile.dep
