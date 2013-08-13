include ../Makefile.inc

all:
	${MAKE} -j${CORES} -C util
	${MAKE} -j${CORES} -C mve
	${MAKE} -j${CORES} -C dmrecon
	${MAKE} -j${CORES} -C ogl 

clean:
	${MAKE} -C util clean
	${MAKE} -C mve clean
	${MAKE} -C dmrecon clean
	${MAKE} -C ogl clean
