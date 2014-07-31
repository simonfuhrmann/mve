all:
	@echo "Apps need to be built separately."
	@echo "For apps requiring qmake: Run 'qmake', then 'make'."

clean:
	$(MAKE) -C bundle2ply $@
	$(MAKE) -C dmrecon $@
	$(MAKE) -C makescene $@
	$(MAKE) -C meshconvert $@
	$(MAKE) -C mveshell $@
	$(MAKE) -C scene2pset $@

.PHONY: all clean
