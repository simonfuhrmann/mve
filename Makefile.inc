# Compiler, Flags, Definitions, ...
CXX ?= g++
CXXFLAGS ?= -Wall -Wextra -Wundef -ansi -pedantic -g -O3
COMPILE.cc = ${CXX} ${CXXFLAGS} ${CPPFLAGS} -c
LINK.o = ${CXX} ${LDFLAGS}
.DEFAULT_GOAL := ${TARGET}

# Always compile programs and libs with multi-threading.
# Position independent code is required for the UMVE plugin system.
CXXFLAGS += -pthread -fPIC

# How to link example from example.o and other files:
%: %.o
	${LINK.o} -o $@ $^ ${LDLIBS}

# How to compile example.o from example.cc:
%.o: %.cc
	${COMPILE.cc} -o $@ $<

# Library paths:
vpath libmve_dmrecon.a ${MVE_ROOT}/libs/dmrecon/
vpath libmve_math.a ${MVE_ROOT}/libs/math/
vpath libmve.a ${MVE_ROOT}/libs/mve/
vpath libmve_ogl.a ${MVE_ROOT}/libs/ogl/
vpath libmve_util.a ${MVE_ROOT}/libs/util/

# Autogenerate dependencies.
Makefile.dep: [^_]*.cc
	${COMPILE.cc} -MM $^ >$@

-include Makefile.dep
