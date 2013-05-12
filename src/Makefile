# Makefile

#MYSQL_INCL = -I/usr/include/mysql
MYSQL_INCL = $(shell mysql_config --include)
MYSQL_LIBS = $(shell mysql_config --libs)
LIBGCRYPT_FLAGS=$(shell libgcrypt-config --cflags)
LIBGCRYPT_LIBS=$(shell libgcrypt-config --libs)

INCLUDES = $(wildcard include/katina/*.h)

CXXFLAGS = -D DEBUG -O0 -g3 -Iinclude

LIBKATINA_FLAGS = -D DEBUG -O0 -g3 -fPIC -pthread -Iinclude $(LIBGCRYPT_FLAGS)
LIBKATINA_LIBS = -lrt -pthread -ldl $(LIBGCRYPT_LIBS)
LIBKATINA_OBJS = rcon.o RemoteClient.o irc.o GUID.o Katina.o PKI.o
#LIBKATINA_OBJS = rcon.o irc.o GUID.o Katina.o PKI.o

LIBRARIES = libkatina.so.0.0.0

KATINA_FLAGS = -D DEBUG -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_OBJS = katina.o rcon.o RemoteClient_v0_x.o irc.o Database.o GUID.o

KATINA_1_FLAGS = -D DEBUG -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_1_LIBS = $(MYSQL_LIBS) -lrt -pthread -ldl $(LIBGCRYPT_LIBS) -L. -lkatina
KATINA_1_OBJS = katina-1.o Database.o

KATINA_IRC_FLAGS = -O3 -g0 -pthread -Iinclude
KATINA_IRC_LIBS = -lrt -pthread
KATINA_IRC_OBJS = katina-irc.o rcon.o irc.o message.o

KATINA_VOTES_FLAGS = -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_VOTES_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_VOTES_OBJS = katina-votes.o

PROGRAMS = katina katina-1 katina-irc katina-votes

all: $(LIBRARIES) $(PROGRAMS)

libkatina.so.0.0.0: $(LIBKATINA_OBJS)
	@echo
	@echo Compiling $@
	g++ -shared -Wl,-soname,libkatina.so.0 -o libkatina.so.0.0.0 $(LIBKATINA_OBJS) $(LIBKATINA_LIBS)
	@ln -f -s libkatina.so.0.0.0 libkatina.so.0.0
	@ln -f -s libkatina.so.0.0 libkatina.so.0
	@ln -f -s libkatina.so.0 libkatina.so
	@echo

katina: $(KATINA_OBJS)
	@echo
	@echo Compiling $@ 
	$(CXX) -o $@ $(KATINA_OBJS) $(KATINA_LIBS)
	@echo
	
katina.o: katina.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_FLAGS) -o $@ $<
	
katina-1: $(KATINA_1_OBJS)
	@echo
	@echo Compiling $@ 
	$(CXX) -o $@ $(KATINA_1_OBJS) $(KATINA_1_LIBS)
	@echo
	
katina-1.o: katina-1.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_1_FLAGS) -o $@ $<
	
katina-irc: $(KATINA_IRC_OBJS)
	@echo
	@echo Compiling $@ 
	$(CXX) -o $@ $(KATINA_IRC_OBJS) $(KATINA_IRC_LIBS)
	@echo
	
katina-irc.o: katina-irc.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_IRC_FLAGS) -o $@ $<

katina-votes: $(KATINA_VOTES_OBJS)
	@echo
	@echo Compiling $@ 
	$(CXX) -o $@ $(KATINA_VOTES_OBJS) $(KATINA_VOTES_LIBS)
	@echo
	
katina-votes.o: katina-votes.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_VOTES_FLAGS) -o $@ $<

%.o: %.cpp include/katina/%.h
	@echo -n "AUTO: "
	$(CXX) -fPIC $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<
	
install: $(PROGRAMS)
	mkdir -p $(HOME)/bin
	for program in $(PROGRAMS); \
	    do cp $$program $(HOME)/bin; \
	done

clean:
	rm -f *.o *.so* $(PROGRAMS) $(LIBRARIES) 

