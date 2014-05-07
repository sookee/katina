# Makefile

src_topdir = ../../src

VER_FLAGS = -D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\"

MYSQL_INCL = $(shell mysql_config --include)
MYSQL_LIBS = $(shell mysql_config --libs)
LIBGCRYPT_FLAGS=$(shell libgcrypt-config --cflags)
LIBGCRYPT_LIBS=$(shell libgcrypt-config --libs)

INCLUDES = $(wildcard $(src_topdir)/include/katina/*.h)

CXXFLAGS = -D DEBUG -O0 -g3 -Iinclude

LIBKATINA_FLAGS = -D DEBUG -O0 -g3 -fPIC -pthread -Iinclude $(LIBGCRYPT_FLAGS)
LIBKATINA_LIBS = -lrt -pthread -ldl $(LIBGCRYPT_LIBS)
LIBKATINA_OBJS = rcon.o RemoteClient.o irc.o GUID.o Katina.o PKI.o

LIBRARIES = libkatina.so

KATINA_FLAGS = -D DEBUG $(VER_FLAGS) -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_LIBS = $(MYSQL_LIBS) -lrt -pthread -ldl $(LIBGCRYPT_LIBS) -L. -lkatina
KATINA_OBJS = katina.o Database.o

KATINA_IRC_FLAGS = -D DEBUG -O0 -g3 -pthread -Iinclude
KATINA_IRC_LIBS = -lrt -pthread
KATINA_IRC_OBJS = katina-irc.o rcon.o irc.o message.o

KATINA_VOTES_FLAGS = -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_VOTES_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_VOTES_OBJS = katina-votes.o

KATINA_GUID_FIX_FLAGS = -D DEBUG -O0 -g3 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread -Iinclude
KATINA_GUID_FIX_LIBS = $(MYSQL_LIBS) -lrt -pthread -ldl -L. -lkatina
KATINA_GUID_FIX_OBJS = katina-guid-fix.o Database.o

PROGRAMS = katina katina-irc katina-votes katina-guid-fix

PREFIX ?= $(HOME)

#ifeq ($(origin PREFIX), defined)
#  PREFIX = -$(PREFIX)
#  echo PREFIX: $(PREFIX)
#endif


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

katina.o: $(src_topdir)/katina.cpp $(INCLUDES)
	$(CXX) -c -I$(src_topdir)/include $(KATINA_FLAGS) -o $@ $<

katina-irc: $(KATINA_IRC_OBJS)
	@echo
	@echo Compiling $@
	$(CXX) -o $@ $(KATINA_IRC_OBJS) $(KATINA_IRC_LIBS)
	@echo

katina-irc.o: $(src_topdir)/katina-irc.cpp $(INCLUDES)
	$(CXX) -c -I$(src_topdir)/include $(KATINA_IRC_FLAGS) -o $@ $<

katina-votes: $(KATINA_VOTES_OBJS)
	@echo
	@echo Compiling $@
	$(CXX) -o $@ $(KATINA_VOTES_OBJS) $(KATINA_VOTES_LIBS)
	@echo

katina-votes.o: $(src_topdir)/katina-votes.cpp $(INCLUDES)
	$(CXX) -c -I$(src_topdir)/include $(KATINA_VOTES_FLAGS) -o $@ $<

katina-guid-fix: $(KATINA_GUID_FIX_OBJS)
	@echo
	@echo Compiling $@
	$(CXX) -o $@ $(KATINA_GUID_FIX_OBJS) $(KATINA_GUID_FIX_LIBS)
	@echo

katina-guid-fix.o: $(src_topdir)/katina-guid-fix.cpp $(INCLUDES)
	$(CXX) -c -I$(src_topdir)/include $(KATINA_GUID_FIX_FLAGS) -o $@ $<

%.o: $(src_topdir)/%.cpp $(src_topdir)/include/katina/%.h
	@echo -n "AUTO: "
	$(CXX) -fPIC -I$(src_topdir)/include $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<

install: $(PROGRAMS) $(LIBRARIES)
	mkdir -p $(PREFIX)/bin
	@for program in $(PROGRAMS); \
	do \
		cp $$program $(PREFIX)/bin; \
	done
	mkdir -p $(PREFIX)/lib
	@for library in $(LIBRARIES); \
	do \
		cp $$library.0.0.0 $(PREFIX)/lib; \
		cd $(PREFIX)/lib; \
		ln -f -s $$library.0.0.0 $$library.0.0; \
		ln -f -s $$library.0.0 $$library.0; \
		ln -f -s $$library.0 $$library; \
	done

clean:
	rm -f *.o *.so* $(PROGRAMS) $(LIBRARIES)

