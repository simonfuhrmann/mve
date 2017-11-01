all:
	$(MAKE) -C bundle2pset
	$(MAKE) -C dmrecon
	$(MAKE) -C makescene
	$(MAKE) -C meshconvert
	$(MAKE) -C scene2pset
	$(MAKE) -C sfmrecon
	$(MAKE) -C featurerecon
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
	$(MAKE) -C featurerecon $@
	$(MAKE) -C fssrecon $@
	$(MAKE) -C mesh2pset $@
	$(MAKE) -C meshalign $@
	$(MAKE) -C meshclean $@
	$(MAKE) -C sceneupgrade $@

BINDIR ?= $(HOME)/bin/
APPDIR := $(shell pwd)

links:
	ln -si $(APPDIR)/bundle2pset/bundle2pset $(BINDIR)
	ln -si $(APPDIR)/dmrecon/dmrecon $(BINDIR)
	ln -si $(APPDIR)/makescene/makescene $(BINDIR)
	ln -si $(APPDIR)/meshconvert/meshconvert $(BINDIR)
	ln -si $(APPDIR)/scene2pset/scene2pset $(BINDIR)
	ln -si $(APPDIR)/sfmrecon/sfmrecon $(BINDIR)
	ln -si $(APPDIR)/featurerecon/featurerecon $(BINDIR)
	ln -si $(APPDIR)/fssrecon/fssrecon $(BINDIR)
	ln -si $(APPDIR)/mesh2pset/mesh2pset $(BINDIR)
	ln -si $(APPDIR)/meshalign/meshalign $(BINDIR)
	ln -si $(APPDIR)/meshclean/meshclean $(BINDIR)
	ln -si $(APPDIR)/sceneupgrade/sceneupgrade $(BINDIR)
	ln -si $(APPDIR)/umve/umve $(BINDIR)

.PHONY: all clean links
