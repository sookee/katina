
DEF_FLAGS = \
	-D _GLIBCXX_USE_NANOSLEEP \
	-D _GLIBCXX_USE_SCHED_YIELD	
DEBUG = 1
	
MYSQL_INCL = -I/usr/include/mysql
MYSQL_LIBS = -L/usr/lib/mysql -lmysqlclient -lz -lm -lrt -lssl -lcrypto -ldl
MYSQL_FLAG = 

PCRE_INCL = $(shell pkg-config libpcrecpp --include)
PCRE_LIBS = $(shell pkg-config libpcrecpp --libs)

ALL_INCL = $(MYSQL_INCL) $(PCRE_INCL)
ALL_LIBS = $(MYSQL_LIBS) $(PCRE_LIBS)

CXX = g++
CC = g++
CXXFLAGS = -std=c++11 $(DEF_FLAGS) $(MYSQL_FLAG) $(ALL_INCL) -pthread
LDFLAGS = $(ALL_LIBS)  -std=c++11 -pthread

INCLUDES = $(wildcard *.h)

all: oastats katina

rcon.o: rcon.h
logrep.o: logrep.h types.h stl.h
str.o: str.h logrep.h types.h stl.h
types.o: types.h stl.h
oastats.o: str.h logrep.h types.h stl.h

oastats: oastats.o logrep.o str.o types.o
katina: katina.o rcon.o logrep.o str.o types.o

clean:
	rm -f *.o *.so oastats katina
