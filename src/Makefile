# Makefile

MYSQL_INCL = -I/usr/include/mysql
MYSQL_LIBS = $(shell mysql_config --libs)

INCLUDES = $(wildcard *.h)

CXXFLAGS = -D DEBUG -O0 -g3
KATINA_FLAGS = -D DEBUG -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread
KATINA_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_OBJS = katina.o rcon.o RemoteClient.o irc.o Database.o GUID.o

KATINA_IRC_FLAGS = -O3 -g0 -pthread
KATINA_IRC_LIBS = -lrt -pthread
KATINA_IRC_OBJS = katina-irc.o rcon.o irc.o message.o

KATINA_VOTES_FLAGS = -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread
KATINA_VOTES_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_VOTES_OBJS = katina-votes.o

all: katina katina-irc katina-votes

katina: $(KATINA_OBJS)
	$(CXX) -o $@ $(KATINA_OBJS) $(KATINA_LIBS)
	
katina.o: katina.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_FLAGS) -o $@ $<
	
katina-irc: $(KATINA_IRC_OBJS)
	$(CXX) -o $@ $(KATINA_IRC_OBJS) $(KATINA_IRC_LIBS)
	
katina-irc.o: katina-irc.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_IRC_FLAGS) -o $@ $<

katina-votes: $(KATINA_VOTES_OBJS)
	$(CXX) -o $@ $(KATINA_VOTES_OBJS) $(KATINA_VOTES_LIBS)
	
katina-votes.o: katina-votes.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_VOTES_FLAGS) -o $@ $<

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<

clean:
	rm -f *.o *.so katina katina-irc
