CXXFLAGS = -std=gnu++98 -pedantic -Wall -g -O0
OUTFILE = ../NEXUS
OBJECTS = config.o netcore.o irc.o state.o server.o nexus.o scrollback.o ignore.o

ifeq ($(OS),WIN)
	CXXFLAGS += -DWIN
	LDFLAGS = -lws2_32 -static
	OUTFILE = ../NEXUS.exe
	OBJECTS += ../win32/win.o
endif

ifeq ($(DEBUG),1)
	CXXFLAGS += -DDEBUG
endif

all: netcorecpp scrollbackcpp ignorecpp nexuscpp servercpp statecpp irccpp configcpp
ifeq ($(OS),WIN)
	$(MAKE) -C ../win32 all
endif
	$(MAKE) -C substrings static
	$(CXX) $(OBJECTS) substrings/libsubstrings.a -o $(OUTFILE) $(LDFLAGS)

netcorecpp:
	$(CXX) netcore.cpp -c $(CXXFLAGS)
scrollbackcpp:
	$(CXX) scrollback.cpp -c $(CXXFLAGS)
ignorecpp:
	$(CXX) ignore.cpp -c $(CXXFLAGS)
nexuscpp:
	$(CXX) nexus.cpp -c $(CXXFLAGS)
servercpp:
	$(CXX) server.cpp -c $(CXXFLAGS)
statecpp:
	$(CXX) state.cpp -c $(CXXFLAGS)
irccpp:
	$(CXX) irc.cpp -c $(CXXFLAGS)
configcpp:
	$(CXX) config.cpp -c $(CXXFLAGS)

clean:
	$(MAKE) -C substrings clean
	rm -f *.o *.gch
