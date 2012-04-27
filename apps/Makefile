all:
	@echo "Apps need to be build separately."
	@echo "For apps requiring qmake: Run 'qmake', then 'make'."

clean:
	$(MAKE) -C bundle2ply clean
	$(MAKE) -C dmrecon clean
	$(MAKE) -C makescene clean
	$(MAKE) -C meshconvert clean
	$(MAKE) -C mveshell clean
	$(MAKE) -C scene2poisson clean
	$(MAKE) -C scene2pset clean

depend:
	$(MAKE) -C bundle2ply depend
	$(MAKE) -C dmrecon depend
	$(MAKE) -C makescene depend
	$(MAKE) -C meshconvert depend
	$(MAKE) -C mveshell depend
	$(MAKE) -C scene2poisson depend
	$(MAKE) -C scene2pset depend
