TARGET := all
MVE_ROOT := ../../
include ${MVE_ROOT}/Makefile.inc

CXXFLAGS += -I${MVE_ROOT}/libs -fopenmp
LDLIBS += -lpng -ltiff -ljpeg -fopenmp -ldmf -lmve -lmve_util -L${MVE_ROOT}/libs/mve/ -L${MVE_ROOT}/libs/util/ -Llibdmfusion

all: dmfoctree dmfslicer dmfsurface dmfextract

dmfoctree: dmfoctree.o libdmfusion/libdmf.a ${MVE_ROOT}/libs/util/libmve_util.a ${MVE_ROOT}/libs/mve/libmve.a
	${CXX} dmfoctree.o ${LDLIBS} -o dmfoctree

dmfslicer: dmfslicer.o libdmfusion/libdmf.a ${MVE_ROOT}/libs/util/libmve_util.a ${MVE_ROOT}/libs/mve/libmve.a
	${CXX} dmfslicer.o ${LDLIBS} -o dmfslicer

dmfsurface: dmfsurface.o libdmfusion/libdmf.a ${MVE_ROOT}/libs/util/libmve_util.a ${MVE_ROOT}/libs/mve/libmve.a
	${CXX} dmfsurface.o ${LDLIBS} -o dmfsurface

tetgen.o: CPPFLAGS += -DTETLIBRARY

dmfextract: dmfextract.o tetgen.o predicates.o libdmfusion/libdmf.a ${MVE_ROOT}/libs/util/libmve_util.a ${MVE_ROOT}/libs/mve/libmve.a
	${CXX} dmfextract.o tetgen.o predicates.o ${LDLIBS} -o dmfextract

libdmfusion/libdmf.a: libdmfusion/*.cc
	${MAKE} -C libdmfusion

clean:
	${RM} dmfoctree dmfslicer dmfsurface dmfextract *.o Makefile.dep
	${MAKE} -C libdmfusion clean

.PHONY: clean
