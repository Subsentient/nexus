CXXFLAGS = -std=gnu++03 -pedantic -Wall -g -O0 -fno-exceptions -fno-rtti
OUTFILE = ../NEXUS
OBJECTS = config.o netcore.o irc.o state.o server.o nexus.o scrollback.o ignore.o

ifeq ($(OS),WIN)
	CXXFLAGS += -lws2_32 -DWIN
	OUTFILE = ../NEXUS.exe
	OBJECTS += ../win32/win.o
endif

ifeq ($(DEBUG),1)
	CXXFLAGS += -DDEBUG
endif

all:
ifeq ($(OS),WIN)
	$(MAKE) -C ../win32 all
endif
	$(MAKE) -C substrings static
	$(MAKE) -C libcpsl
	$(CXX) config.cpp -c $(CXXFLAGS)
	$(CXX) netcore.cpp -c $(CXXFLAGS)
	$(CXX) irc.cpp -c $(CXXFLAGS)
	$(CXX) state.cpp -c $(CXXFLAGS)
	$(CXX) server.cpp -c $(CXXFLAGS)
	$(CXX) nexus.cpp -c $(CXXFLAGS)
	$(CXX) scrollback.cpp -c $(CXXFLAGS)
	$(CXX) ignore.cpp -c $(CXXFLAGS)
	$(CC) $(OBJECTS) substrings/libsubstrings.a libcpsl/libcpsl.a -o $(OUTFILE) $(LDFLAGS)


clean:
	$(MAKE) -C substrings clean
	$(MAKE) -C libcpsl clean
	rm -f *.o *.gch
