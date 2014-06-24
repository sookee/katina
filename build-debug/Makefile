-include Makefile.local

all:
	$(MAKE) -C src
#	$(MAKE) -C src/oacom
	$(MAKE) -C src/plugins
	
clean:
	$(MAKE) -C src clean
#	$(MAKE) -C src/oacom clean
	$(MAKE) -C src/plugins clean
	
install:
	$(MAKE) -C src install
#	$(MAKE) -C src/oacom install
	$(MAKE) -C src/plugins install

