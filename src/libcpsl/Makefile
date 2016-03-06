CC = gcc
AR = ar
RANLIB = ranlib
CFLAGS = -std=gnu99 -pedantic -Wall -O0 -g3
PREFIX = /usr
OBJECTS = linkedlist.o dynarray.o settings.o texthash.o

all: linkedlist dynarray settings texthash
	@echo ""
	@echo "Building libcpsl.a..."
	$(AR) rc libcpsl.a $(OBJECTS)
	$(RANLIB) libcpsl.a

linkedlist:
	$(CC) -c linkedlist.c $(CFLAGS)
	
dynarray:
	$(CC) -c dynarray.c $(CFLAGS)
	
settings:
	$(CC) -c settings.c $(CFLAGS)
	
texthash:
	$(CC) -c texthash.c $(CFLAGS)

install-hdr:
	mkdir -p $(PREFIX)/include/
	install -v -m 0644 libcpsl.h $(PREFIX)/include/

install-static:
	mkdir -p $(PREFIX)/lib/
	install -v -m 0644 libcpsl.a $(PREFIX)/lib/

install-all: install-hdr install-static

install: install-all

uninstall:
	rm -vf $(PREFIX)/include/libcpsl.h
	rm -vf $(PREFIX)/lib/libcpsl.*

test:
	@echo "Checking if libcpsl.a is compiled..."
	@echo ""
	@test -e libcpsl.a
	@tests/starttests.sh "$(CC)" "$(CFLAGS)"
	@echo ""
	@echo "All tests passed."
	
clean:
	rm -vf *.a
	rm -vf *.o
	rm -vf libcpsl*.so*
	rm -vf *.gch
	rm -vf tests/*.o
	rm -vf tests/*.gch
	rm -vf tests/bin/*
distclean: clean
