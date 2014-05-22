# Makefile

src_topdir = ../../../src

MYSQL_INCL = -I/usr/include/mysql
MYSQL_LIBS = $(shell mysql_config --libs)

INCLUDES = $(wildcard *.h)

CXXFLAGS = -fPIC -D DEBUG -O0 -g3 -I$(src_topdir)/include

PLUGIN_STATS_INCLUDES = -I..

#DL_EXPORT = -Wl,-E
DL_EXPORT =

PREFIX ?= $(HOME)

PLUGIN_EXAMPLE_FLAGS = -D DEBUG -O0 -g3 $(DL_EXPORT) -shared -pthread
PLUGIN_EXAMPLE_LIBS = -pthread -L.. -lkatina
PLUGIN_EXAMPLE_OBJS = KatinaPluginExample.o

PLUGIN_STATS_FLAGS = -D DEBUG -O0 -g3 $(DL_EXPORT) -shared -pthread
PLUGIN_STATS_LIBS = -pthread -L.. -lkatina
PLUGIN_STATS_OBJS = KatinaPluginStats.o ../Database.o

PLUGIN_REPORTS_FLAGS = -D DEBUG -O0 -g3 $(DL_EXPORT) -shared -pthread
PLUGIN_REPORTS_LIBS = -pthread -L.. -lkatina
PLUGIN_REPORTS_OBJS = KatinaPluginReports.o

PLUGIN_VOTES_FLAGS = -D DEBUG -O0 -g3 $(DL_EXPORT) -shared -pthread
PLUGIN_VOTES_LIBS = -pthread -L.. -lkatina
PLUGIN_VOTES_OBJS = KatinaPluginVotes.o ../Database.o

PLUGIN_WSO_FLAGS = -D DEBUG -O0 -g3 $(DL_EXPORT) -shared -pthread
PLUGIN_WSO_LIBS = -pthread -L.. -lkatina
PLUGIN_WSO_OBJS = WinnerStaysOn.o

PLUGIN_VOTECTRL_FLAGS = -D DEBUG -O0 -g3 $(DL_EXPORT) -shared -pthread
PLUGIN_VOTECTRL_LIBS = -pthread -L.. -lkatina
PLUGIN_VOTECTRL_OBJS = KatinaPluginCallVoteCtrl.o

PLUGIN_TEAMBALANCER_FLAGS = -D DEBUG -O0 -g3 $(DL_EXPORT) -shared -pthread
PLUGIN_TEAMBALANCER_LIBS = -pthread -L.. -lkatina
PLUGIN_TEAMBALANCER_OBJS = KatinaPluginTeamBalancer.o TB_DefaultEvaluation.o TB_DefaultTeamBuilder.o ../Database.o

PLUGIN_PLAYER_DB_FLAGS = -D DEBUG -O0 -g3 $(DL_EXPORT) -shared -pthread
PLUGIN_PLAYER_DB_LIBS = -pthread -L.. -lkatina
PLUGIN_PLAYER_DB_OBJS = KatinaPluginPlayerDb.o ../Database.o

PLUGIN_ADMIN_FLAGS = -D DEBUG -O0 -g3 $(DL_EXPORT) -shared -pthread
PLUGIN_ADMIN_LIBS = -pthread -L.. -lkatina
PLUGIN_ADMIN_OBJS = KatinaPluginAdmin.o

PLUGINS = \
	katina-plugin-admin.so \
	katina-plugin-stats.so \
	katina-plugin-votes.so \
	katina-plugin-reports.so \
	katina-plugin-wso.so \
	katina-plugin-votectrl.so \
	katina-plugin-teambalancer.so \
	katina-plugin-player-db.so \
	katina-plugin-example.so

all: $(PLUGINS)

katina-plugin-example.so: $(PLUGIN_EXAMPLE_OBJS)
	$(CXX) $(PLUGIN_EXAMPLE_FLAGS) -o $@ $(PLUGIN_EXAMPLE_OBJS) $(PLUGIN_EXAMPLE_LIBS)

katina-plugin-admin.so: $(PLUGIN_ADMIN_OBJS)
	$(CXX) $(PLUGIN_ADMIN_FLAGS) -o $@ $(PLUGIN_ADMIN_OBJS) $(PLUGIN_ADMIN_LIBS)

katina-plugin-stats.so: $(PLUGIN_STATS_OBJS)
	$(CXX) $(PLUGIN_STATS_FLAGS) -o $@ $(PLUGIN_STATS_OBJS) $(PLUGIN_STATS_LIBS)

katina-plugin-votes.so: $(PLUGIN_VOTES_OBJS)
	$(CXX) $(PLUGIN_VOTES_FLAGS) -o $@ $(PLUGIN_VOTES_OBJS) $(PLUGIN_VOTES_LIBS)

katina-plugin-reports.so: $(PLUGIN_REPORTS_OBJS)
	$(CXX) $(PLUGIN_REPORTS_FLAGS) -o $@ $(PLUGIN_REPORTS_OBJS) $(PLUGIN_REPORTS_LIBS)

katina-plugin-wso.so: $(PLUGIN_WSO_OBJS)
	$(CXX) $(PLUGIN_WSO_FLAGS) -o $@ $(PLUGIN_WSO_OBJS) $(PLUGIN_WSO_LIBS)

katina-plugin-votectrl.so: $(PLUGIN_VOTECTRL_OBJS)
	$(CXX) $(PLUGIN_VOTECTRL_FLAGS) -o $@ $(PLUGIN_VOTECTRL_OBJS) $(PLUGIN_VOTECTRL_LIBS)
	
katina-plugin-teambalancer.so: $(PLUGIN_TEAMBALANCER_OBJS)
	$(CXX) $(PLUGIN_TEAMBALANCER_FLAGS) -o $@ $(PLUGIN_TEAMBALANCER_OBJS) $(PLUGIN_TEAMBALANCER_LIBS)

katina-plugin-player-db.so: $(PLUGIN_PLAYER_DB_OBJS)
	$(CXX) $(PLUGIN_PLAYER_DB_FLAGS) -o $@ $(PLUGIN_PLAYER_DB_OBJS) $(PLUGIN_PLAYER_DB_LIBS)

%.o: $(src_topdir)/plugins/%.cpp $(src_topdir)/plugins/%.h
	$(CXX) $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<

install: $(PLUGINS)
	mkdir -p $(PREFIX)/lib
	for plugin in $(PLUGINS); do cp -f $$plugin $(PREFIX)/lib; done

clean:
	rm -f *.o *.so $(PLUGINS)

