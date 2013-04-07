
MYSQL_INCL = -I/usr/include/mysql
MYSQL_LIBS = $(shell mysql_config --libs)

INCLUDES = $(wildcard *.h)

KATINA_FLAGS = -O3 -g0 $(MYSQL_FLAG) $(MYSQL_INCL) -pthread
KATINA_LIBS = $(MYSQL_LIBS) -lrt -pthread

all: katina

katina: katina.o
	$(CXX) -o $@ $< $(KATINA_LIBS)
	
katina.o: katina.cpp $(INCLUDES)
	$(CXX) -c $(KATINA_FLAGS) -o $@ $<

rcon.o: rcon.cpp rcon.h
	$(CXX) -c -o $@ $<

clean:
	rm -f *.o *.so katina
