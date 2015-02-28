# Makefile

src_topdir = ../../src

VER_FLAGS = \
	-D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\" \
	-D DEV=\"$(shell git diff --quiet || echo -dev)\"

# requirements
REQ_CXX11_TEST := $(shell bash -e echo "int main(){}" | $(CXX) -o /dev/null -x c++ -std=c++11 - 2> /dev/null; echo $$?)
REQ_CXX11_FAIL := "ERROR: Requires g++ v4.7 or above to compile."
REQ_CXX11 = if(($(REQ_CXX11_TEST))); then echo $(REQ_CXX11_FAIL); exit 1; fi

VER_GXX := 4.7
REQ_GXX_TEST := $(shell echo -e "$(VER_GXX)\n$(shell g++ --version|head -1|cut -d' ' -f3)"|sort -V|head -1)
REQ_GXX_FAIL := "ERROR: Requires g++ v$(VER_GXX) or above."
REQ_GXX = if [[ $(REQ_GXX_TEST) != $(VER_GXX) ]]; then echo $(REQ_GXX_FAIL); exit 1; fi

VER_PCREL := 7.8
REQ_PCRE_TEST := $(shell echo -e "$(VER_PCREL)\n$(shell pcre-config --version)"|sort -V|head -1)
REQ_PCRE_FAIL := "ERROR: Requires PCRE v$(VER_PCREL) or above."
REQ_PCRE = if [[ $(REQ_PCRE_TEST) != $(VER_PCREL) ]]; then echo $(REQ_PCRE_FAIL); exit 1; fi

VER_MYSQL := 5.5.37
REQ_MYSQL_TEST := $(shell echo -e "$(VER_MYSQL)\n$(shell mysql_config --version)"|sort -V|head -1)
REQ_MYSQL_FAIL := "ERROR: Requires MySQL v$(VER_MYSQL) or above."
REQ_MYSQL = if [[ $(REQ_MYSQL_TEST) != $(VER_MYSQL) ]]; then echo $(REQ_MYSQL_FAIL); exit 1; fi

# libraries
PCRE_INCL = $(shell pcre-config --cflags)
PCRE_LIBS = $(shell pcre-config --libs-cpp)
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

PROGRAMS = katina katina-votes katina-obscure-ips

PREFIX ?= $(HOME)

#all: reqs $(LIBRARIES) $(PROGRAMS)
all: $(LIBRARIES) $(PROGRAMS)

reqs:
	@$(REQ_CXX11)
	@$(REQ_PCRE)
	@$(REQ_MYSQL)

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

katina-obscure-ips: katina-obscure-ips.o
	@echo Linking $@
	@$(CXX) -o $@ $^ $(PCRE_LIBS)

katina-obscure-ips.o: $(src_topdir)/katina-obscure-ips.cpp $(INCLUDES)
	@echo Compiling $@
	@$(CXX) -c -I$(src_topdir)/include $(KATINA_FLAGS) $(PCRE_INCL) -o $@ $<

#katina-irc: $(KATINA_IRC_OBJS)
#	@echo Compiling $@
#	@$(CXX) -o $@ $(KATINA_IRC_OBJS) $(KATINA_IRC_LIBS)
#
#katina-irc.o: $(src_topdir)/katina-irc.cpp $(INCLUDES)
#	@echo Compiling $@
#	@$(CXX) -c -I$(src_topdir)/include $(KATINA_IRC_FLAGS) -o $@ $<

katina-votes: $(KATINA_VOTES_OBJS)
	@echo Compiling $@
	@$(CXX) -o $@ $(KATINA_VOTES_OBJS) $(KATINA_VOTES_LIBS)

katina-votes.o: $(src_topdir)/katina-votes.cpp $(INCLUDES)
	@echo Compiling $@
	@$(CXX) -c -I$(src_topdir)/include $(KATINA_VOTES_FLAGS) -o $@ $<

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

.PHONY: reqs

