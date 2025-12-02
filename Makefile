all:
	$(MAKE) -C source

install:
	$(MAKE) -C source install

clean:
	$(MAKE) -C source clean

.PHONY: all install clean
