all:
	$(MAKE) -C libs
	$(MAKE) -C apps

doc:
	$(MAKE) -C docs

clean:
	$(MAKE) -C apps clean
	$(MAKE) -C libs clean
	#$(MAKE) -C docs clean
