
DEF_FLAGS = \
	-D DEBUG \
	-D _GLIBCXX_USE_NANOSLEEP \
	-D _GLIBCXX_USE_SCHED_YIELD	
DEBUG = 1
	
#MYSQL_INCL = -I/usr/include/mysql
#MYSQL_LIBS = -L/usr/lib/mysql -lmysqlclient -lz -lm -lrt -lssl -lcrypto -ldl
MYSQL_INCL = 
MYSQL_LIBS = 
MYSQL_FLAG = 

PCRE_INCL = $(shell pkg-config libpcrecpp --cflags)
PCRE_LIBS = $(shell pkg-config libpcrecpp --libs)

ALL_INCL = $(MYSQL_INCL) $(PCRE_INCL)
ALL_LIBS = $(MYSQL_LIBS) $(PCRE_LIBS)

KATINA_FLAGS = $(DEF_FLAGS) $(MYSQL_FLAG) $(ALL_INCL) -pthread
OASTATS_FLAGS = -std=c++11 $(DEF_FLAGS) $(MYSQL_FLAG) $(ALL_INCL) -pthread

KATINA_LIBS = $(MYSQL_LIBS) $(PCRE_LIBS) -lrt -pthread
OASTATS_LIBS = $(MYSQL_LIBS) $(PCRE_LIBS) -pthread

CXX = g++
CC = g++
CXXFLAGS = $(ALL_INCL)
#CXXFLAGS = -std=c++11 $(DEF_FLAGS) $(MYSQL_FLAG) $(ALL_INCL) -pthread
#LDFLAGS = $(ALL_LIBS) -pthread

INCLUDES = $(wildcard *.h)

#all: oastats katina
all: katina

oastats: oastats.o logrep.o str.o types.o
	$(CXX) -o $@ $< $(OASTATS_FLAGS)

katina: katina.o
	$(CXX) -o $@ $< $(KATINA_LIBS)
	
katina.o: katina.cpp
	$(CXX) -c $(KATINA_FLAGS) -o $@ $<

rcon.o: rcon.cpp rcon.h
	$(CXX) -c -o $@ $<

logrep.o: logrep.cpp logrep.h types.h stl.h
	$(CXX) -c -std=c++11 -o $@ $<

str.o: str.cpp str.h logrep.h types.h stl.h
	$(CXX) -c -std=c++11 -o $@ $<

types.o: types.cpp types.h stl.h
	$(CXX) -c -std=c++11 -o $@ $<

oastats.o: oastats.cpp str.h logrep.h types.h stl.h
	$(CXX) -c -std=c++11 $(OASTATS_FLAGS) -o $@ $<

clean:
	rm -f *.o *.so oastats katina
