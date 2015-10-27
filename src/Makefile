CFLAGS = -std=gnu99 -pedantic -Wall -g -O0
OUTFILE = ../NEXUS
OBJECTS = config.c netcore.c irc.c state.c server.c nexus.c scrollback.c ignore.c

ifeq ($(OS),WIN)
	CFLAGS += -lws2_32 -DWIN
	OUTFILE = ../NEXUS.exe
	OBJECTS += ../win32/win.o
endif

all:
ifeq ($(OS),WIN)
	$(MAKE) -C ../win32 all
endif
	$(MAKE) -C substrings static
	$(CC) $(OBJECTS) substrings/libsubstrings.a -o $(OUTFILE) $(CFLAGS)
clean:
	$(MAKE) -C substrings clean
	rm -f *.o *.gch
