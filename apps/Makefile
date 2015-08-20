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
	ln -sir apps/bundle2pset/bundle2pset $(HOME)/bin/
	ln -sir apps/dmrecon/dmrecon $(HOME)/bin/
	ln -sir apps/makescene/makescene $(HOME)/bin/
	ln -sir apps/meshconvert/meshconvert $(HOME)/bin/
	ln -sir apps/scene2pset/scene2pset $(HOME)/bin/
	ln -sir apps/sfmrecon/sfmrecon $(HOME)/bin/
	ln -sir apps/fssrecon/fssrecon $(HOME)/bin/
	ln -sir apps/mesh2pset/mesh2pset $(HOME)/bin/
	ln -sir apps/meshalign/meshalign $(HOME)/bin/
	ln -sir apps/meshclean/meshclean $(HOME)/bin/
	ln -sir apps/sceneupgrade/sceneupgrade $(HOME)/bin/
	ln -sir apps/umve/umve $(HOME)/bin/

.PHONY: all clean links
