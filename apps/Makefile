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
	$(MAKE) -C sceneupgrade

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
	$(MAKE) -C sceneupgrade $@

links:
	ln -sir bundle2pset/bundle2pset $(HOME)/bin/
	ln -sir dmrecon/dmrecon $(HOME)/bin/
	ln -sir makescene/makescene $(HOME)/bin/
	ln -sir meshconvert/meshconvert $(HOME)/bin/
	ln -sir scene2pset/scene2pset $(HOME)/bin/
	ln -sir sfmrecon/sfmrecon $(HOME)/bin/
	ln -sir fssrecon/fssrecon $(HOME)/bin/
	ln -sir mesh2pset/mesh2pset $(HOME)/bin/
	ln -sir meshalign/meshalign $(HOME)/bin/
	ln -sir meshclean/meshclean $(HOME)/bin/
	ln -sir sceneupgrade/sceneupgrade $(HOME)/bin/
	ln -sir umve/umve $(HOME)/bin/

.PHONY: all clean links
