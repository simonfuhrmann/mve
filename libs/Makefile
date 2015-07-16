all:
	${MAKE} -C math
	${MAKE} -C util
	${MAKE} -C mve
	${MAKE} -C sfm
	${MAKE} -C dmrecon
	$(MAKE) -C fssr
	${MAKE} -C ogl

clean:
	${MAKE} -C math $@
	${MAKE} -C util $@
	${MAKE} -C mve $@
	${MAKE} -C sfm $@
	${MAKE} -C dmrecon $@
	$(MAKE) -C fssr $@
	${MAKE} -C ogl $@

.PHONY: all clean
