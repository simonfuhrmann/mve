all:
	$(MAKE) -C libs
	$(MAKE) -C apps

doc:
	$(MAKE) -C docs

clean:
	$(MAKE) -C apps $@
	$(MAKE) -C libs $@
	#$(MAKE) -C docs $@

.PHONY: all doc clean
