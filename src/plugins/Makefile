# Makefile

MYSQL_INCL = -I/usr/include/mysql
MYSQL_LIBS = $(shell mysql_config --libs)

INCLUDES = $(wildcard *.h)

CXXFLAGS = -fPIC -D DEBUG -O0 -g3 -I../include

PLUGIN_STATS_INCLUDES = -I..

PLUGIN_EXAMPLE_FLAGS = -D DEBUG -O0 -g3 -Wl,-E -shared -pthread
PLUGIN_EXAMPLE_LIBS = -pthread -L.. -lkatina
PLUGIN_EXAMPLE_OBJS = KatinaPluginExample.o

PLUGIN_STATS_FLAGS = -D DEBUG -O0 -g3 -Wl,-E -shared -pthread
PLUGIN_STATS_LIBS = -pthread -L.. -lkatina
PLUGIN_STATS_OBJS = KatinaPluginStats.o ../Database.o

PLUGINS = katina-plugin-example.so katina-plugin-stats.so

all: $(PLUGINS)

katina-plugin-example.so: $(PLUGIN_EXAMPLE_OBJS)
	$(CXX) $(PLUGIN_EXAMPLE_FLAGS) -o $@ $(PLUGIN_EXAMPLE_OBJS) $(PLUGIN_EXAMPLE_LIBS)

katina-plugin-stats.so: $(PLUGIN_STATS_OBJS)
	$(CXX) $(PLUGIN_STATS_FLAGS) -o $@ $(PLUGIN_STATS_OBJS) $(PLUGIN_STATS_LIBS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<
	
install: $(PLUGINS)
	mkdir -p $(HOME)/lib
	for plugin in $(PLUGINS); do cp $$plugin $(HOME)/lib; done

clean:
	rm -f *.o *.so $(PLUGINS)

