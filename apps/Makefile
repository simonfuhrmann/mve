all:
	$(MAKE) -C bundle2pset
	$(MAKE) -C dmrecon
	$(MAKE) -C makescene
	$(MAKE) -C meshconvert
	$(MAKE) -C scene2pset
	$(MAKE) -C sfmrecon
	$(MAKE) -C fssrecon
	$(MAKE) -C mesh2pset
	$(MAKE) -C meshalign
	$(MAKE) -C meshclean
	$(MAKE) -C viewupgrade

clean:
	$(MAKE) -C bundle2pset $@
	$(MAKE) -C dmrecon $@
	$(MAKE) -C makescene $@
	$(MAKE) -C meshconvert $@
	$(MAKE) -C scene2pset $@
	$(MAKE) -C sfmrecon $@
	$(MAKE) -C fssrecon $@
	$(MAKE) -C mesh2pset $@
	$(MAKE) -C meshalign $@
	$(MAKE) -C meshclean $@
	$(MAKE) -C viewupgrade $@

.PHONY: all clean
