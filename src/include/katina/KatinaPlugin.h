#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_H
#define	_OASTATS_KATINA_PLUGIN_H
/*
 * File:   KatinaPlugin.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 5:08 AM
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@gmail.com               |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

#include <memory>
#include <vector>

//#include <boost/shared_ptr.hpp>

#include "types.h"
#include "GUID.h"

namespace oastats {

using namespace oastats::types;

class KatinaPlugin
{
	friend class Katina;

private:
	void* dl;

protected:
	class Katina& katina;

public:
	KatinaPlugin(Katina& katina): dl(0), katina(katina) {}
	virtual ~KatinaPlugin() {}

	/**
	 * This provides an opportunity for a plugin to initialise
	 * itself.
	 *
	 * @return false on failure
	 */
	virtual bool open() = 0;

	/**
	 * Return the unique id of the plugin.
	 */
	virtual str get_id() const = 0;

	/**
	 * Return the name of the plugin.
	 */
	virtual str get_name() const = 0;

	/**
	 * Return the version of the plugin.
	 */
	virtual str get_version() const = 0;
	
	// Game server log events
	virtual bool init_game(siz min, siz sec, const str_map& cvars) {}
	virtual bool warmup(siz min, siz sec) {}
	virtual bool client_connect(siz min, siz sec, siz num) {}
	virtual bool client_begin(siz min, siz sec, siz num) {}
	virtual bool client_disconnect(siz min, siz sec, siz num) {}
	virtual bool client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name) {}
    virtual bool client_switch_team(siz min, siz sec, siz num, siz teamBefore, siz teamNow) {}
	virtual bool kill(siz min, siz sec, siz num1, siz num2, siz weap) {}
	virtual bool ctf(siz min, siz sec, siz num, siz team, siz act) {}
	
	/**
	 * Final score of complete CTF game
	 */
	virtual bool ctf_exit(siz min, siz sec, siz r, siz b) {}
	virtual bool score_exit(siz min, siz sec, int score, siz ping, siz num, const str& name) {}
	virtual bool award(siz min, siz sec, siz num, siz awd) {}
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text) {}
	virtual bool sayteam(siz min, siz sec, const GUID& guid, const str& text) {}
	virtual bool shutdown_game(siz min, siz sec) {}
	virtual bool exit(siz min, siz sec) {}
	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params) {}
	
	/**
	 * Summarizing events for more detailed statistics (they only work with the katina game mod)
	 */
	virtual bool weapon_usage(siz min, siz sec, siz num, siz weapon, siz shots) {}
	virtual bool mod_damage(siz min, siz sec, siz num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits) {}
	virtual bool player_stats(siz min, siz sec, siz num,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged) {}
	 
	 
	/**
	 * This provides an opportunity for a plugin to clean
	 * itself up. It is called before the plugin is removed/reloaded.
	 * This is a good place to clean up any threads, close files etc.
	 */
	virtual void close() = 0;
    
    virtual void heartbeat(siz min, siz sec) {}
};

//typedef boost::shared_ptr<KatinaPlugin> KatinaPluginSPtr;
typedef std::vector<KatinaPlugin*> plugin_vec;
typedef plugin_vec::iterator plugin_vec_iter;
typedef plugin_vec::const_iterator plugin_vec_citer;

typedef std::map<str, KatinaPlugin*> plugin_map;
typedef plugin_map::value_type plugin_map_vt;
typedef plugin_map::iterator plugin_map_iter;
typedef plugin_map::const_iterator plugin_map_citer;

/**
 * The plugin implementation source should
 * use this macro to make itself loadable as
 * a plugin.
 *
 * The macro variable should be set to the name
 * of the plugin class to instantiate.
 *
 * The plugin class should derive from KatinaPlugin.
 *
 */
#define KATINA_PLUGIN_TYPE(type) \
extern "C" KatinaPlugin* katina_plugin_factory(Katina& katina) \
{ \
	return new type(katina); \
} extern int _missing_semicolon_()

/**
 * Plugins should use this macro which provides
 * an interface to plugin loaders.
 */
#define KATINA_PLUGIN_INFO(I, N, V) \
static const char* ID = I; \
static const char* NAME = N; \
static const char* VERSION = V

/**
 * Please use plog() rather than log() in your plugins
 */
#define plog(m) log(ID << ": " << m)

} // oastats

#endif // _OASTATS_KATINA_PLUGIN_H

