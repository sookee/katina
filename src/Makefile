# Makefile

MYSQL_INCL = -I/usr/include/mysql
MYSQL_LIBS = $(shell mysql_config --libs)

INCLUDES = $(wildcard *.h)

CXXFLAGS = -O0 -g3 -D DEBUG

KATINA_SCAN_FLAGS = $(CXXFLAGS) $(MYSQL_FLAG) $(MYSQL_INCL) -pthread
KATINA_SCAN_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_SCAN_OBJS = katina-scan.o Katina.o rcon.o RemoteIRCClient.o irc.o Database.o GUID.o rconthread.o

KATINA_IRCBOT_FLAGS = -O3 -g0 -pthread
KATINA_IRCBOT_LIBS = -lrt -pthread
KATINA_IRCBOT_OBJS = katina-ircbot.o rcon.o irc.o message.o

KATINA_VOTES_FLAGS = -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread
KATINA_VOTES_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_VOTES_OBJS = katina-votes.o

PROGRAMS = katina-scan katina-ircbot katina-votes

all: $(PROGRAMS)

katina-scan: $(KATINA_SCAN_OBJS)
	$(CXX) -o $@ $(KATINA_SCAN_OBJS) $(KATINA_SCAN_LIBS)
	
katina-scan.o: katina-scan.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_SCAN_FLAGS) -o $@ $<
	
katina-ircbot: $(KATINA_IRCBOT_OBJS)
	$(CXX) -o $@ $(KATINA_IRCBOT_OBJS) $(KATINA_IRCBOT_LIBS)
	
katina-ircbot.o: katina-ircbot.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_IRCBOT_FLAGS) -o $@ $<

katina-votes: $(KATINA_VOTES_OBJS)
	$(CXX) -o $@ $(KATINA_VOTES_OBJS) $(KATINA_VOTES_LIBS)
	
katina-votes.o: katina-votes.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_VOTES_FLAGS) -o $@ $<

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<

clean:
	rm -f *.o *.so $(PROGRAMS)
