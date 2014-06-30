# Makefile

src_topdir = ../../src

VER_FLAGS = -D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\"

MYSQL_INCL = $(shell mysql_config --include)
MYSQL_LIBS = $(shell mysql_config --libs)
LIBGCRYPT_FLAGS=$(shell libgcrypt-config --cflags)
LIBGCRYPT_LIBS=$(shell libgcrypt-config --libs)

INCLUDES = $(wildcard $(src_topdir)/include/katina/*.h)

CXXFLAGS = -std=c++11 $(DEBUG_FLAGS) -Iinclude -pthread

LIBKATINA_FLAGS = $(CXXFLAGS) $(VER_FLAGS) -fPIC -pthread $(LIBGCRYPT_FLAGS)
LIBKATINA_LIBS = -lrt -pthread -ldl $(LIBGCRYPT_LIBS)
LIBKATINA_OBJS = ansi.o rcon.o RemoteClient.o irc.o GUID.o PKI.o types.o Database.o Katina.o radp.o

KATINA_FLAGS = $(CXXFLAGS) $(VER_FLAGS) $(MYSQL_FLAG) $(MYSQL_INCL)
KATINA_LIBS = $(MYSQL_LIBS) -lrt -pthread -ldl $(LIBGCRYPT_LIBS) -L. -lkatina
KATINA_OBJS = katina.o

KATINA_IRC_FLAGS = $(CXXFLAGS) -pthread
KATINA_IRC_LIBS = -lrt -pthread
KATINA_IRC_OBJS = katina-irc.o rcon.o irc.o message.o

KATINA_VOTES_FLAGS = $(CXXFLAGS) $(MYSQL_FLAG) $(MYSQL_INCL)
KATINA_VOTES_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_VOTES_OBJS = katina-votes.o

KATINA_WHOIS_FLAGS = $(CXXFLAGS) $(MYSQL_FLAG) $(MYSQL_INCL)
KATINA_WHOIS_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_WHOIS_OBJS = katina-whois.o

KATINA_CLIENT_FLAGS = $(CXXFLAGS) $(MYSQL_FLAG) $(MYSQL_INCL)
KATINA_CLIENT_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_CLIENT_OBJS = katina-client.o

LIBRARIES = libkatina.so

PROGRAMS = katina

PREFIX ?= $(HOME)

all: $(LIBRARIES) $(PROGRAMS)

libkatina.so: $(LIBKATINA_OBJS)
	@echo Linking $@
	@g++ $(CXXFLAGS) -shared -Wl,-soname,$@.0 -o $@.0.0.0 $(LIBKATINA_OBJS) $(LIBKATINA_LIBS)
	@ln -f -s $@.0.0.0 $@.0.0
	@ln -f -s $@.0.0 $@.0
	@ln -f -s $@.0 $@

katina: libkatina.so $(KATINA_OBJS)
	@echo Linking $@
	@$(CXX) -o $@ $(KATINA_OBJS) $(KATINA_LIBS)

katina.o: $(src_topdir)/katina.cpp $(INCLUDES)
	@echo Compiling $@
	@$(CXX) -c -I$(src_topdir)/include $(KATINA_FLAGS) -o $@ $<

#katina-irc: $(KATINA_IRC_OBJS)
#	@echo Compiling $@
#	@$(CXX) -o $@ $(KATINA_IRC_OBJS) $(KATINA_IRC_LIBS)
#
#katina-irc.o: $(src_topdir)/katina-irc.cpp $(INCLUDES)
#	@echo Compiling $@
#	@$(CXX) -c -I$(src_topdir)/include $(KATINA_IRC_FLAGS) -o $@ $<

#katina-votes: $(KATINA_VOTES_OBJS)
#	@echo Compiling $@
#	@$(CXX) -o $@ $(KATINA_VOTES_OBJS) $(KATINA_VOTES_LIBS)
#
#katina-votes.o: $(src_topdir)/katina-votes.cpp $(INCLUDES)
#	@echo Compiling $@
#	@$(CXX) -c -I$(src_topdir)/include $(KATINA_VOTES_FLAGS) -o $@ $<

#katina-whois: $(KATINA_WHOIS_OBJS)
#	@echo Compiling $@
#	@$(CXX) -o $@ $(KATINA_WHOIS_OBJS) $(KATINA_WHOIS_LIBS)
#
#katina-whois.o: $(src_topdir)/katina-whois.cpp $(INCLUDES)
#	@echo Compiling $@
#	@$(CXX) -c -I$(src_topdir)/include $(KATINA_WHOIS_FLAGS) -o $@ $<

#katina-client: $(KATINA_CLIENT_OBJS)
#	@echo
#	@echo Compiling $@
#	$(CXX) -o $@ $(KATINA_CLIENT_OBJS) $(KATINA_CLIENT_LIBS)
#	@echo
#
#katina-client.o: $(src_topdir)/katina-client.cpp $(INCLUDES)
#	$(CXX) -c -I$(src_topdir)/include $(KATINA_CLIENT_FLAGS) -o $@ $<

%.o: $(src_topdir)/%.cpp $(src_topdir)/include/katina/*.h
	@echo Compiling $@
	@$(CXX) -fPIC -I$(src_topdir)/include $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<

install: $(PROGRAMS) $(LIBRARIES)
	@mkdir -p $(PREFIX)/bin
	@for program in $(PROGRAMS); \
	do \
		echo Installing $$program to $(PREFIX)/bin; \
		cp -f $$program $(PREFIX)/bin; \
	done
	@mkdir -p $(PREFIX)/lib
	@for library in $(LIBRARIES); \
	do \
		echo Installing $$library to $(PREFIX)/lib; \
		cp -f $$library.0.0.0 $(PREFIX)/lib; \
		cd $(PREFIX)/lib; \
		ln -f -s $$library.0.0.0 $$library.0.0; \
		ln -f -s $$library.0.0 $$library.0; \
		ln -f -s $$library.0 $$library; \
	done

clean:
	rm -f *.o *.so* $(PROGRAMS) $(LIBRARIES)

