include ../Makefile.inc

all:
	${MAKE} -j${CORES} -C math
	${MAKE} -j${CORES} -C util
	${MAKE} -j${CORES} -C mve
	${MAKE} -j${CORES} -C dmrecon
	${MAKE} -j${CORES} -C ogl

clean:
	${MAKE} -C math $@
	${MAKE} -C util $@
	${MAKE} -C mve $@
	${MAKE} -C dmrecon $@
	${MAKE} -C ogl $@

.PHONY: all clean
