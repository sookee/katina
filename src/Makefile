# Makefile

VER_FLAGS = -D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\"

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

LIBRARIES = libkatina.so

KATINA_FLAGS = -D DEBUG -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_OBJS = katina.o rcon.o RemoteClient_v0_x.o irc.o Database.o GUID.o

KATINA_1_FLAGS = -D DEBUG $(VER_FLAGS) -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_1_LIBS = $(MYSQL_LIBS) -lrt -pthread -ldl $(LIBGCRYPT_LIBS) -L. -lkatina
KATINA_1_OBJS = katina-1.o Database.o

KATINA_IRC_FLAGS = -D DEBUG -O0 -g3 -pthread -Iinclude
KATINA_IRC_LIBS = -lrt -pthread
KATINA_IRC_OBJS = katina-irc.o rcon.o irc.o message.o

KATINA_VOTES_FLAGS = -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_VOTES_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_VOTES_OBJS = katina-votes.o

KATINA_GUID_FIX_FLAGS = -D DEBUG -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_GUID_FIX_LIBS = $(MYSQL_LIBS) -lrt -pthread -ldl -L. -lkatina
KATINA_GUID_FIX_OBJS = katina-guid-fix.o Database.o

PROGRAMS = katina katina-1 katina-irc katina-votes katina-guid-fix

all: $(addsuffix .0.0.0, $(LIBRARIES)) $(PROGRAMS)

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

katina.o: katina_old.cpp $(INCLUDES)
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

katina-guid-fix: $(KATINA_GUID_FIX_OBJS)
	@echo
	@echo Compiling $@
	$(CXX) -o $@ $(KATINA_GUID_FIX_OBJS) $(KATINA_GUID_FIX_LIBS)
	@echo

katina-guid-fix.o: katina-guid-fix.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_GUID_FIX_FLAGS) -o $@ $<

%.o: %.cpp include/katina/%.h
	@echo -n "AUTO: "
	$(CXX) -fPIC $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<

install: $(PROGRAMS) $(LIBRARIES)
	mkdir -p $(HOME)/bin
	@for program in $(PROGRAMS); \
	do \
		cp $$program $(HOME)/bin; \
	done
	mkdir -p $(HOME)/lib
	@for library in $(LIBRARIES); \
	do \
		cp $$library.0.0.0 $(HOME)/lib; \
		cd $(HOME)/lib; \
		ln -f -s $$library.0.0.0 $$library.0.0; \
		ln -f -s $$library.0.0 $$library.0; \
		ln -f -s $$library.0 $$library; \
	done

clean:
	rm -f *.o *.so* $(PROGRAMS) $(LIBRARIES)

