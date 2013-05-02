# Makefile

MYSQL_INCL = -I/usr/include/mysql
MYSQL_LIBS = $(shell mysql_config --libs)

INCLUDES = $(wildcard include/katina/*.h)

CXXFLAGS = -D DEBUG -O0 -g3 -Iinclude

LIBKATINA_FLAGS = -D DEBUG -O0 -g3 -fPIC -pthread -Iinclude
LIBKATINA_LIBS = -lrt -pthread -ldl
LIBKATINA_OBJS = rcon.o RemoteClient.o irc.o GUID.o Katina.o

LIBRARIES = libkatina.so

KATINA_FLAGS = -D DEBUG -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_OBJS = katina.o rcon.o RemoteClient.o irc.o Database.o GUID.o

KATINA_1_FLAGS = -D DEBUG -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_1_LIBS = $(MYSQL_LIBS) -lrt -pthread -ldl -L. -lkatina
KATINA_1_OBJS = katina-1.o Database.o

KATINA_IRC_FLAGS = -O3 -g0 -pthread -Iinclude
KATINA_IRC_LIBS = -lrt -pthread
KATINA_IRC_OBJS = katina-irc.o rcon.o irc.o message.o

KATINA_VOTES_FLAGS = -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_VOTES_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_VOTES_OBJS = katina-votes.o

PROGRAMS = katina katina-1 katina-irc katina-votes

all: $(LIBRARIES) $(PROGRAMS)

libkatina.so: $(LIBKATINA_OBJS)
	g++ -shared -Wl,-soname,libkatina.so.0 -o libkatina.so.0.0.0 $(LIBKATINA_OBJS) $(KATINA_LIBS)
	@ln -f -s libkatina.so.0.0.0 libkatina.so.0.0
	@ln -f -s libkatina.so.0.0 libkatina.so.0
	@ln -f -s libkatina.so.0 libkatina.so

katina: $(KATINA_OBJS)
	$(CXX) -o $@ $(KATINA_OBJS) $(KATINA_LIBS)
	
katina.o: katina.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_FLAGS) -o $@ $<
	
katina-1: $(KATINA_1_OBJS)
	$(CXX) -o $@ $(KATINA_1_OBJS) $(KATINA_1_LIBS)
	
katina-1.o: katina-1.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_1_FLAGS) -o $@ $<
	
katina-irc: $(KATINA_IRC_OBJS)
	$(CXX) -o $@ $(KATINA_IRC_OBJS) $(KATINA_IRC_LIBS)
	
katina-irc.o: katina-irc.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_IRC_FLAGS) -o $@ $<

katina-votes: $(KATINA_VOTES_OBJS)
	$(CXX) -o $@ $(KATINA_VOTES_OBJS) $(KATINA_VOTES_LIBS)
	
katina-votes.o: katina-votes.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_VOTES_FLAGS) -o $@ $<

%.o: %.cpp include/katina/%.h
	@echo -n "AUTO: "
	$(CXX) -fPIC $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<
	
install: $(PROGRAMS)
	mkdir -p $(HOME)/bin
	for program in $(PROGRAMS); do cp $$program $(HOME)/bin; done

clean:
	rm -f *.o *.so $(PROGRAMS)

