CXXFLAGS = -std=c++11 -pedantic -Wall -g -O0
OUTFILE = ../NEXUS
OBJECTS = config.cpp netcore.cpp irc.cpp state.cpp server.cpp nexus.cpp scrollback.cpp ignore.cpp

ifeq ($(OS),WIN)
	CXXFLAGS += -lws2_32 -DWIN
	OUTFILE = ../NEXUS.exe
	OBJECTS += ../win32/win.o
endif

all:
ifeq ($(OS),WIN)
	$(MAKE) -C ../win32 all
endif
	$(MAKE) -C substrings static
	$(CXX) $(OBJECTS) substrings/libsubstrings.a -o $(OUTFILE) $(CXXFLAGS)
clean:
	$(MAKE) -C substrings clean
	rm -f *.o *.gch
