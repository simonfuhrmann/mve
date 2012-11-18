include ../Makefile.inc

all:
	${MAKE} -j${CORES} -C util
	${MAKE} -j${CORES} -C mve
	${MAKE} -j${CORES} -C sfm
	${MAKE} -j${CORES} -C dmrecon
	${MAKE} -j${CORES} -C ogl 

clean:
	${MAKE} -C util clean
	${MAKE} -C mve clean
	${MAKE} -C sfm clean
	${MAKE} -C dmrecon clean
	${MAKE} -C ogl clean

depend:
	${MAKE} -C util depend
	${MAKE} -C mve depend
	${MAKE} -C sfm depend
	${MAKE} -C dmrecon depend
	${MAKE} -C ogl depend

