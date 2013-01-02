
GCC_HOME = $(HOME)/app/gcc-4.7.0

DEF_FLAGS = \
	-D _GLIBCXX_USE_NANOSLEEP \
	-D _GLIBCXX_USE_SCHED_YIELD \
	-D STAMP=\"$(shell date +%Y%m%d-%H%M%S)\" \
	-D DEV=\"$(shell git diff --quiet || echo -dev)\" \
	-D COMMITS=\"$(shell printf %04d \"$(git log --after={yesterday} --pretty=oneline|wc -l)\")\" \
	-D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\"
	
MYSQL_INCL = $(shell mysql_config --include)
MYSQL_LIBS = $(shell mysql_config --libs)

PCRE_INCL = $(pkg-config libpcrecpp --include)
PCRE_LIBS = $(pkg-config libpcrecpp --libs)

ALL_INCL = $(MYSQL_INCL) $(PCRE_INCL)
ALL_LIBS = $(MYSQL_LIBS) $(PCRE_LIBS)

CXX = $(GCC_HOME)/bin/g++
CC = $(GCC_HOME)/bin/g++
CXXFLAGS = -std=c++11 $(DEF_FLAGS) $(ALL_INCL)
LDFLAGS = $(ALL_LIBS)

INCLUDES = $(wildcard *.h)

all: oastats

oastats.o: $(INCLUDES)

clean:
	rm -f *.o *.so oastats
