# Makefile

MYSQL_INCL = -I/usr/include/mysql
MYSQL_LIBS = $(shell mysql_config --libs)

INCLUDES = $(wildcard *.h)

KATINA_FLAGS = -O3 -g0 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread
KATINA_LIBS = $(MYSQL_LIBS) -lrt -pthread
KATINA_OBJS = katina.o rcon.o

all: katina

katina: $(KATINA_OBJS)
	$(CXX) -o $@ $(KATINA_OBJS) $(KATINA_LIBS)
	
katina.o: katina.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_FLAGS) -o $@ $<

clean:
	rm -f *.o *.so katina
