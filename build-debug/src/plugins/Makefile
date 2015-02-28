# Makefile

src_topdir = ../../../src

MYSQL_INCL = -I/usr/include/mysql
MYSQL_LIBS = $(shell mysql_config --libs)

INCLUDES = $(wildcard *.h)

CXXFLAGS = -std=c++11 -fPIC $(DEBUG_FLAGS) -I$(src_topdir)/include

#DL_EXPORT = -Wl,-E
DL_EXPORT = 

PREFIX ?= $(HOME)

PLUGIN_LIBS = -pthread -L.. -lkatina
PLUGIN_FLAGS = $(CXXFLAGS) $(DL_EXPORT) -shared -pthread

PLUGINS = \
	katina-plugin-admin.so \
	katina-plugin-stats.so \
	katina-plugin-votes.so \
	katina-plugin-nextmap.so \
	katina-plugin-reports.so \
	katina-plugin-votectrl.so \
	katina-plugin-playerdb.so

UNUSED_PLUGINS = \
	katina-plugin-example.so \
	katina-plugin-teambalancer.so \
	katina-plugin-wso.so
	
	TESTS = test-stats-stall-clients
	
all: $(PLUGINS) $(TESTS)

test-stats-stall-clients: $(src_topdir)/plugins/test-stats-stall-clients.cpp
	@echo Building $@
	@$(CXX) $(CXXFLAGS) -o $@ $^

katina-plugin-example.so: KatinaPluginExample.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)

katina-plugin-admin.so: KatinaPluginAdmin.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)

katina-plugin-stats.so: KatinaPluginStats.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)

katina-plugin-votes.so: KatinaPluginVotes.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)

katina-plugin-nextmap.so: KatinaPluginNextMap.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)

katina-plugin-reports.so: KatinaPluginReports.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)

katina-plugin-wso.so: WinnerStaysOn.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)

katina-plugin-votectrl.so: KatinaPluginCallVoteCtrl.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)
	
katina-plugin-teambalancer.so: KatinaPluginTeamBalancer.o TB_DefaultEvaluation.o TB_DefaultTeamBuilder.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)

katina-plugin-playerdb.so: KatinaPluginPlayerDb.o
	@echo Linking $@
	@$(CXX) $(PLUGIN_FLAGS) -o $@ $^ $(PLUGIN_LIBS)

%.o: $(src_topdir)/plugins/%.cpp $(src_topdir)/plugins/%.h $(src_topdir)/include/katina/*.h
	@echo Compiling $@
	@$(CXX) $(CXXFLAGS) $(MYSQL_INCL) -c -o $@ $<

install: $(PLUGINS)
	@mkdir -p $(PREFIX)/lib
	@for plugin in $(PLUGINS); \
	do \
		echo Installing $$plugin to $(PREFIX)/lib; \
		cp -f $$plugin $(PREFIX)/lib; \
	done

clean:
	rm -f *.o *.so $(PLUGINS)

