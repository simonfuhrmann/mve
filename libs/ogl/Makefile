include ../../Makefile.inc

SOURCES := $(wildcard [^_]*.cc)
OBJECTS := $(foreach file,$(SOURCES),$(subst .cc,.o,$(file)))
LIBRARY := libogl.a

EXT_INCL := -I..

${LIBRARY}: ${OBJECTS}
	ar rcs $@ $^
	chmod a+x $@

%.o: %.cc
	${CXX} -c -o $@ $< ${CXXFLAGS} ${EXT_INCL}

Makefile.dep: ${SOURCES}
	${CXX} -MM ${SOURCES} ${EXT_INCL} >$@

clean:
	${RM} ${OBJECTS} ${LIBRARY} Makefile.dep

.PHONY: clean

-include Makefile.dep
