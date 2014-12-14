all:
	$(MAKE) -C src all
clean:
	rm -fv NEXUS
	$(MAKE) -C src clean
	$(MAKE) -C win32 clean
install:
	install -m 755 NEXUS /usr/bin/