include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cpp)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cpp,.o,$(file)))
LIBRARY := libdmrecon.a

EXT_INCL := -I..
EXT_LIBS := -L../util -L../mve -lmve -lutil -lpng -ljpeg -ltiff

libmve: ${OBJECTS}
	ar rcs ${LIBRARY} ${OBJECTS}
	chmod a+x ${LIBRARY}

%.o: %.cpp
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

depend:
	${CXX} -MM ${SOURCES} ${EXT_INCL} > Makefile.dep

clean: FORCE
	${RM} ${OBJECTS} ${LIBRARY} ${TESTBIN}

FORCE:

-include Makefile.dep
