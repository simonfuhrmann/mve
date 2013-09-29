QMAKE := qmake

all:
#	@echo "Apps need to be build separately."
#	@echo "For apps requiring qmake: Run 'qmake', then 'make'."
	cd umve && $(QMAKE)
	$(MAKE) -C bundle2ply
	$(MAKE) -C dmrecon
	$(MAKE) -C makescene
	$(MAKE) -C meshconvert
	$(MAKE) -C mveshell
#	$(MAKE) -C scene2poisson
	$(MAKE) -C scene2pset
	$(MAKE) -C umve
	$(MAKE) -C dmfusion

clean:
	$(MAKE) -C bundle2ply $@
	$(MAKE) -C dmrecon $@
	$(MAKE) -C makescene $@
	$(MAKE) -C meshconvert $@
	$(MAKE) -C mveshell $@
	#$(MAKE) -C scene2poisson $@
	$(MAKE) -C scene2pset $@
	$(MAKE) -C dmfusion $@

install:
	$(MAKE) -C bundle2ply $@
	$(MAKE) -C dmrecon $@
	$(MAKE) -C makescene $@
	$(MAKE) -C meshconvert $@
	$(MAKE) -C mveshell $@
	#$(MAKE) -C scene2poisson $@
	$(MAKE) -C scene2pset $@
	$(MAKE) -C dmfusion $@

.PHONY: all clean install
