all:
	$(MAKE) -C libs
	$(MAKE) -C apps

clean:
	$(MAKE) -C libs clean
	$(MAKE) -C apps clean
