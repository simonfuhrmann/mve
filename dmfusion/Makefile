TARGET := all
MVE_ROOT := ../
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I${MVE_ROOT}/libs -fopenmp
LDLIBS += -lpng -ltiff -ljpeg -lm -fopenmp -lmve -lutil -L${MVE_ROOT}/libs/util/ -L${MVE_ROOT}/libs/mve/

all: dmfoctree dmfslicer dmfsurface dmfextract

dmfoctree: dmfoctree.o libdmfusion/libdmf.a
	${CC} dmfoctree.o libdmfusion/libdmf.a ${LDLIBS} -o dmfoctree

dmfslicer: dmfslicer.o libdmfusion/libdmf.a
	${CC} dmfslicer.o libdmfusion/libdmf.a ${LDLIBS} -o dmfslicer

dmfsurface: dmfsurface.o libdmfusion/libdmf.a
	${CC} dmfsurface.o libdmfusion/libdmf.a ${LDLIBS} -o dmfsurface

tetgen.o: CPPFLAGS += -DTETLIBRARY
dmfextract: dmfextract.o tetgen.o predicates.o libdmfusion/libdmf.a

libdmfusion/libdmf.a: libdmfusion/*.cc
	${MAKE} -C libdmfusion

clean:
	${RM} dmfoctree dmfslicer dmfsurface dmfextract *.o Makefile.dep
	${MAKE} -C libdmfusion clean

.PHONY: clean
