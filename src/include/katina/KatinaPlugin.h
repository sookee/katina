//#pragma once
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

namespace katina {

using namespace katina::types;

/**
 * This is the abstract base class for all plugins.
 */
class KatinaPlugin
{
	friend class Katina;

private:
	void* dl;
	bool opened = false;
	unsigned priority = 0; // 0 = high

protected:
	class Katina& katina;

public:
	KatinaPlugin(Katina& katina): dl(0), katina(katina) {}
	virtual ~KatinaPlugin() {}

	bool is_loaded() const { return dl; }
	bool is_open() const { return opened; }

	/**
	 * return list of plugin ids for plugind that this plugin
	 * optionally or otherwise depends on to be processed before
	 * this one.
	 */
	virtual str_vec get_parent_plugin_ids() const { return {}; };

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
	
	/**
	 * Interface for other plugins to use
	 */
	virtual str api(const str& cmd, void* blob = nullptr) { return "ERROR: unknown request"; }

	// Game server log events

	/**
	 * Called at the very beginning of each game.
	 *
	 * When this function is called NONE of the core data structures
	 * (clients, players, teams) contain any data.
	 *
	 * They will fill up on subsequent ClientUserinfoChanged: events.
	 */
	virtual bool init_game(siz min, siz sec, const str_map& svars) { return true; }

	/**
	 * Called IMMEDIATELY after init_game() IF this is a warmup game.
	 *
	 * Therefore the core data structures (clients, players, teams)
	 * don't contain any data.
	 *
	 * They will fill up on subsequent ClientUserinfoChanged: events.
	 */
	virtual bool warmup(siz min, siz sec) { return true; }

	virtual bool client_connect(siz min, siz sec, slot num) { return true; }

	/**
	 * Only with mod_katina >= 0.1-beta
	 * Only reliable with mod_katina >= 0.1.1 (Is this true?)
	 */
	virtual bool client_connect_info(siz min, siz sec, slot num, const GUID& guid, const str& ip) { return true; }

	// TODO: can these be used to validate client_connect_info info?
	virtual bool client_begin(siz min, siz sec, slot num) { return true; }
	virtual bool client_disconnect(siz min, siz sec, slot num) { return true; }
	virtual bool client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name, siz hc) { return true; }
    virtual bool client_switch_team(siz min, siz sec, slot num, siz teamBefore, siz teamNow) { return true; }
	virtual bool kill(siz min, siz sec, slot num1, slot num2, siz weap) { return true; }
	virtual bool push(siz min, siz sec, slot num1, slot num2) { return true; }

	/**
	 *
	 * @param min
	 * @param sec
	 * @param num slot::bad = flag returned after timaout
	 * @param team
	 * @param act
	 * @return
	 */
	virtual bool ctf(siz min, siz sec, slot num, siz team, siz act) { return true; }
	
	/**
	 * Final score of complete CTF game
	 */
	virtual bool ctf_exit(siz min, siz sec, siz r, siz b) { return true; }
	virtual bool score_exit(siz min, siz sec, int score, siz ping, slot num, const str& name) { return true; }
	virtual bool award(siz min, siz sec, slot num, siz awd) { return true; }
//	virtual bool say(siz min, siz sec, slot num, const str& text) { return true; }
	virtual bool say(siz min, siz sec, slot num, const str& text) { return true; }
	virtual bool sayteam(siz min, siz sec, slot num, const str& text) { return true; }
	virtual bool chat(siz min, siz sec, const str& text) { return true; }
	virtual bool shutdown_game(siz min, siz sec) { return true; }
	virtual bool exit(siz min, siz sec) { return true; }
	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params) { return true; }
	
	/**
	 * Only with mod_katina >= 0.1-beta
	 */
	virtual bool callvote(siz min, siz sec, slot num, const str& type, const str& info) { return true; }

	/**
	 *  Only with mod_katina >= 0.1-beta
	 * @param has_flag if true this speed record is calculated ONLY whn carrying the flag
	 */
	virtual bool speed(siz min, siz sec, slot num, siz dist, siz time, bool has_flag) { return true; }

	/**
	 * Summarizing events for more detailed statistics (they only work with the katina game mod)
	 */
	virtual bool weapon_usage(siz min, siz sec, slot num, siz weapon, siz shots) { return true; }
	virtual bool mod_damage(siz min, siz sec, slot num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits) { return true; }
	virtual bool player_stats(siz min, siz sec, slot num,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged) { return true; }
	 
	 
	/**
	 * This provides an opportunity for a plugin to clean
	 * itself up. It is called before the plugin is removed/reloaded.
	 * This is a good place to clean up any threads, close files etc.
	 */
	virtual void close() = 0;
    
	/**
	 * Potentially every time a message arrives in the log file
	 * a HEARTBEAT event can be sent to the plugin (depending
	 * on the "regularity" provided by the plugin's override
	 * of the get_regularity() function.
	 *
	 * The default regularity (in seconds) is 0 (meaning never).
	 *
	 * Therefore plugins implementing this function should
	 * also implement the get_regularity() function to tell
	 * katina how often to call the heartbeat in seconds.
	 */
    virtual void heartbeat(siz min, siz sec) {}

    /**
     * Override this function to contrl how often (in seconds)
     * the heartbeat() function is called.
     *
     * Unless this function is overriden, it returns 0 meaning that
     * the heartbeat() function will never be called.
     *
     * A return value of 2 will mean the heartbeat() event will
     * be called once every two seconds. A return value of 5 means
     * the heartbeat() function will be called once every five seconds, etc.
     *
     * @param time_in_secs The number of seconds since the first InitGame:
     * for the currently running game. This is ((min * 60) + sec) as taken
     * from the log file being processed.
     */
    virtual siz get_regularity(siz time_in_secs) const { return 0; }
};

typedef std::shared_ptr<KatinaPlugin> KatinaPluginSPtr;
typedef std::unique_ptr<KatinaPlugin> KatinaPluginUPtr;

typedef std::vector<KatinaPluginUPtr> plugin_uptr_vec;
typedef std::vector<KatinaPlugin*> plugin_vec;
typedef plugin_vec::iterator plugin_vec_iter;
typedef plugin_vec::const_iterator plugin_vec_citer;

TYPEDEF_MAP(str, KatinaPlugin*, plugin_map);

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
#define PREPASTER(x, y) x ## y
#define PASTER(x, y) PREPASTER(x, y)

#define KATINA_PLUGIN_TYPE(type) \
extern "C" KatinaPlugin* katina_plugin_factory(Katina& katina) \
{ \
	return new type(katina); \
} struct _missing_semicolon_{}
/**
 * Plugins should use this macro which provides
 * an interface to plugin loaders.
 */
#define KATINA_PLUGIN_INFO(I, N, V) \
static const str ID = I; \
static const str NAME = N; \
static const str VERSION = V

/**
 * Please use plog() rather than log() in your plugins
 */
#define plog(m) log(ID << ": " << m)

#define ptlog(m) plog("[" << std::this_thread::get_id() << "] " << m)

/**
 * Please use pbug() rather than bug() in your plugins
 */
#define pbug(m) bug(ID << ": " << m)

#define ptbug(m) pbug("[" << std::this_thread::get_id() << "] " << m)

/**
 * Please use pbug_var() rather than bug_var() in your plugins
 */
#define pbug_var(v) pbug(QUOTE(v:) << std::boolalpha << " " << v)

#define ptbug_var(v) pbug("[" << std::this_thread::get_id() << "] " << QUOTE(v:) << std::boolalpha << " " << v)

template<typename T>
void set_blob(void* blob, T& t)
{
	*static_cast<T**>(blob) = &t;
}

template<typename T>
void* set_blob(T*& t)
{
	return &t;
}

} // katina

#endif // _OASTATS_KATINA_PLUGIN_H

