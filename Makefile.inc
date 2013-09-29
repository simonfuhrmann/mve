ifeq (${OS},Windows_NT)
  PLATFORM = win32
else
  SHELL = /bin/sh
  PLATFORM = $(shell uname|tr '[:upper:]' '[:lower:]')
endif

# Compiler to use if not specified otherwise
CXX ?= g++

# Number of cores in the system
ifeq (${PLATFORM},linux)
  CORES = $(shell grep processor /proc/cpuinfo | wc -l)
else ifeq (${PLATFORM},freebsd)
  CORES = $(shell sysctl -n hw.ncpu)
else ifeq (${PLATFORM},darwin)
  CORES_LINE = $(shell system_profiler SPHardwareDataType | grep Cores)
  CORES = $(lastword $(CORES_LINE))
else
  CORES = 1
endif

# Compiler flags and defines
GCCFLAGS ?= -Wall -Wextra -Wundef -ansi -pedantic -g -O3
GCCDEFINES ?=
CXXFLAGS = ${GCCFLAGS} ${GCCDEFINES}

# Common commands
MKDIR := mkdir -p
CP    := cp -f
