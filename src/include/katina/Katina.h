//#pragma once
#ifndef _OASTATS_KATINA_H
#define	_OASTATS_KATINA_H

/*
 * File:   Katina.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 1, 2013, 6:23 PM
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

#include "KatinaPlugin.h"
#include "GUID.h"
#include "rcon.h"
#include "types.h"
#include "utils.h"
#include "PKI.h"
#include "log.h"
#include "radp.h"

#include <list>
//#include <pthread.h>
#include <memory>
#include <map>
#include <array>
#include <future>

int main(const int argc, const char* argv[]);

namespace katina {

using namespace katina::log;
using namespace katina::net;
using namespace katina::pki;
using namespace katina::types;
using namespace katina::utils;

enum event_t
{
	LOG_NONE
	, INIT_GAME
	, WARMUP
	, CLIENT_CONNECT
	, CLIENT_BEGIN
	, CLIENT_DISCONNECT
	, CLIENT_USERINFO_CHANGED
    , CLIENT_SWITCH_TEAM
	, KILL
	, CTF
	, CTF_EXIT
	, SCORE_EXIT
	, AWARD
	, SAY
	, SAYTEAM
	, CHAT
	, SHUTDOWN_GAME
	, EXIT
	, UNKNOWN
    , HEARTBEAT
	, WEAPON_USAGE // mod_katina >= 0.1-beta
	, MOD_DAMAGE // mod_katina >= 0.1-beta
	, PLAYER_STATS // mod_katina >= 0.1-beta
    , SPEED //mod_katina >= 0.1-beta
    , PUSH // mod_katina >= 0.1-beta
	, CLIENT_CONNECT_INFO // mod_katina >= 0.1-beta
	, LOG_CALLVOTE// mod_katina >= 0.1-beta
};

TYPEDEF_MAP(event_t, plugin_lst, event_map);
//typedef std::map<event_t, plugin_vec> event_map;
//typedef event_map::iterator event_map_iter;
//typedef event_map::const_iterator event_map_citer;

struct cvar
{
	virtual ~cvar() {}
	virtual bool get(str& s) const = 0;
	virtual bool set(const str& s) = 0;
};

sis& operator>>(sis& i, siz_set& s);
sos& operator<<(sos& o, const siz_set& s);

template<typename T>
class cvar_t
: public cvar
{
	T& t;

public:
	cvar_t(T& t): t(t) {}

	virtual bool get(str& s) const
	{
		soss oss;
		if(!(oss << t))
			return false;
		s = oss.str();
		return true;
	}
	virtual bool set(const str& s)
	{
		T t = T();
		siss iss(s);
		if(!(iss >> t))
			return false;
		this->t = t;
		return true;
	}
};

template<>
class cvar_t<str>
: public cvar
{
	str& s;

public:
	cvar_t(str& s): s(s) {}

	virtual bool get(str& s) const
	{
		s = this->s;
		return true;
	}
	virtual bool set(const str& s)
	{
		this->s = s;
		return true;
	}
};

typedef std::shared_ptr<cvar> cvar_sptr;
typedef std::unique_ptr<cvar> cvar_uptr;

typedef std::map<str, cvar_uptr> cvar_map;
typedef cvar_map::iterator cvar_map_iter;
typedef cvar_map::const_iterator cvar_map_citer;
typedef cvar_map::value_type cvar_map_pair;

typedef std::map<KatinaPlugin*, cvar_map> cvar_map_map;
typedef cvar_map_map::iterator cvar_map_map_iter;
typedef cvar_map_map::const_iterator cvar_map_map_citer;
typedef cvar_map_map::value_type cvar_map_map_pair;

inline
sis& operator>>(sis& i, siz_set& s)
{
	siz v;
	while(i >> v)
		s.insert(v);
	return i;
}

inline
sos& operator<<(sos& o, const siz_set& s)
{
	str sep;
	for(siz_set_citer i = s.begin(); i != s.end(); ++i)
		{ o << sep << *i; sep = " "; }
	return o;
}

// const rad chk_Kill = line_data + sizeof("Kill");
// 15:13 Speed: 0 9622 45 : Client 0 ran 9622u in 45s without the flag.
#define EVT_PARSE_INFO(e) \
const rad chk_ ## e = line_data + 7 + sizeof(QUOTE(e)) - 1; \
const rad param_ ## e = chk_ ## e + 2

/**
 * This is the main log-file processing class.
 */
class Katina
{
	friend int ::main(const int argc, const char* argv[]);
private:
	bool done;
	bool active;

	TYPEDEF_MAP(str, str_vec, property_map);
	TYPEDEF_LST(slot, slot_lst);

	str name;
	str plugin;
	property_map props;

	plugin_map plugins; // id -> KatinaPlugin*
	str_map plugin_files; // id -> filename (for reloading))

	event_map events; // event -> plugin_lst
	cvar_map_map vars; // plugin* -> {name -> cvar*}

	GUID guid_from_name(const str& name);
	GUID extract_name(const str& line);//, str& text);
    
	bool load_config(const str& dir, const str& file, property_map& props);
    bool init_pki();
    void init_rcon();
    
    void load_plugins();
	bool load_plugin(const str& file);
	bool open_plugin(const str& id);
	bool unload_plugin(const str& id);
	bool reload_plugin(const str& id);
    
	bool initial_player_info();
	bool read_backlog(const str& logname, std::ios::streampos pos);
	void builtin_command(const GUID& guid, const str& text);

	// We try to keep map keys GUID based as slot numbers are defunct as soon
	// as a client disconnects.

	// should only need to sync writing to data structures in Katina
	// and reading from them in threads
	std::mutex mtx;
    std::array<bool, MAX_CLIENTS> connected;
	slot_guid_map clients; // slot -> GUID // cleared when players disconnect and on game_begin()
	guid_str_map players; // GUID -> name  // cleard before game_begin()
	guid_siz_map teams; // GUID -> 0,1,2,3 // cleared when players disconnect and on game_begin()

	const static siz BUFFSIZE = 1024; // log buffer size

	/**
	 * Location of the configuration folder.
	 * Typically something like $HOME/.katina
	 */
	str config_dir;

	/**
	 * The current map name
	 */
	str mapname;

	/**
	 * The current map timestamp
	 */
	str timestamp;

	siz line_number = 0; // log file line number
	char line_data[BUFFSIZE]; // log file lines read into this variable
	const rad cmd = line_data + 7;

	EVT_PARSE_INFO(Kill);

	EVT_PARSE_INFO(Award);

	EVT_PARSE_INFO(Challenge);
	EVT_PARSE_INFO(CTF);
	EVT_PARSE_INFO(ClientUserinfoChanged);
	EVT_PARSE_INFO(ClientBegin);
	EVT_PARSE_INFO(ClientConnect);
	EVT_PARSE_INFO(ClientDisconnect);
	EVT_PARSE_INFO(ClientConnectInfo);
	EVT_PARSE_INFO(Callvote);

	EVT_PARSE_INFO(Speed);
	EVT_PARSE_INFO(SpeedFlag);
	EVT_PARSE_INFO(ShutdownGame);

	EVT_PARSE_INFO(WeaponUsage);
	EVT_PARSE_INFO(Warmup);

	EVT_PARSE_INFO(MODDamage);

	EVT_PARSE_INFO(client);
	EVT_PARSE_INFO(chat);

	EVT_PARSE_INFO(say);
	EVT_PARSE_INFO(sayteam);

	EVT_PARSE_INFO(Push);
	EVT_PARSE_INFO(PlayerStats);

	EVT_PARSE_INFO(score);

	EVT_PARSE_INFO(Item);
	EVT_PARSE_INFO(Info);
	EVT_PARSE_INFO(InitGame);

	EVT_PARSE_INFO(red) - 1;

	EVT_PARSE_INFO(Exit);

	bool do_log_lines = false;

	bool live = false;
	bool rerun = false;
	bool backlog = false;

	TYPEDEF_LST(std::future<void>, future_lst);
	//std::mutex futures_mtx;
	future_lst futures;

public:
	Katina();
	~Katina();

	std::mutex& get_data_mutex() { return mtx; }

	void add_future(std::future<void> fut)
	{
		futures.emplace_back(std::move(fut));
	}

	// API

	/**
	 * General purpose encryption/decryption signatures etc...
	 */
	PKI pki;

	/**
	 * Send rcon commands to the game server
	 */
	RCon server;

	/**
	 * Get line number in log file currently being processed
	 */
	siz get_line_number() { return line_number; }

	void log_lines(bool status) { if((do_log_lines = status)) nlog("LINE: " << line_data); }

	/**
	 * Directory of the configuration file.
	 */
	const str& get_config_dir() { return config_dir; }

	/**
	 * Current map being played.
	 */
	const str& get_mapname() { return mapname; }

	/**
	 * Find out if the player with the given GUID
	 * recently disconnected from the server.
	 */
	bool is_disconnected(const GUID& guid) const
	{
		return !guid.is_connected();
	}

	/**
	 * DEFINITIVELY if a client slot is connected or not.
	 */
	bool is_connected(slot num)
	{
		return connected[siz(num)];
	}

	str_map svars; // server variables
	str runmode;
	siz logmode;
	std::time_t now;

	str mod_katina; // server enhancements

	const str& get_runmode() const { return runmode; }
	bool is_live() const { return live; }

	/**
	 * Get the name of the bot (default Katina)
	 */
	const str& get_name() const { return name; }

	/**
	 * Get a read-only reference to the clients data
	 * structure that maps slot numbers to GUIDs.
	 */
	const slot_guid_map& getClients() const { return clients; }

//	/**
//	 * Get a copy of the clients data
//	 * structure that maps slot numbers to GUIDs.
//	 *
//	 * NOTE: This will become out-of-date when used from
//	 * another thread.
//	 *
//	 */
//	slot_guid_map copyClients() { return clients; }

	/**
	 * Get a read-only reference to the players data
	 * structure that maps GUIDs to player names.
	 */
	const guid_str_map& getPlayers() { return players; }

	/**
	 * Get a read-only reference to the teams data
	 * structure that maps GUIDs to player's team.
	 *
	 * 0 = TEAM_U (unknown), 1 = TEAM_R (red), 2 = TEAM_B (blue), 3 = TEAM_S (specs)
	 */
	const guid_siz_map& getTeams() { return teams; }

	/**
	 * Get a cvar's value using rcon
	 */
	bool rconset(const str& cvar, str& val);

    siz getTeam(slot num) const;
    siz getTeam(const GUID& guid) const;
    str getPlayerName(slot num) const;
    str getPlayerName(const GUID& guid) const;
    slot getClientSlot(const GUID& guid) const;
    const GUID& getClientGuid(slot num) const;

    /**
     * return the slot number of the current player from
     * user input that is EITHER the slot number, the GUID
     * OR the name of the player
     *
     * @param slot_guid_name The user input to parse
     * @param num The return parameter.
     *
     * @return true on success
     */
    bool parse_slot_guid_name(const str& slot_guid_name, slot& num);

    bool check_slot(slot num) const
    {
    	return clients.find(num) != clients.end();
    }

	str get_version() const;

	/**
	 * Get a generic plugin pointer, else nullptr if not found.
	 *
	 * @return A pointer to the plugin with the supplied id
	 * else nullptr if not loaded or id not recognized.
	 */
	KatinaPlugin* get_plugin(const str& id, const str& version);

	/**
	 * Get a fully qualified typed pointer to a plugin, else
	 * nullptr if not found or id is not recognized.
	 *
	 * @throws May throw a std::bad_cast if dynamic_cast() fails.
	 */
	template<typename Plugin>
	bool get_plugin(const str& id, const str& version, Plugin*& plugin)
	{
		return (plugin = dynamic_cast<Plugin*>(get_plugin(id, version)));
	}

	/**
	 * Get a type-converted config variable's value.
	 *
	 * @param s The config variable whose value is sought.
	 * @param dflt A default value to use if the variable was not
	 * found in the config file.
	 *
	 * @return The value that the variable s is set to in the config
	 * file else dflt if not present.
	 */
	template<typename T>
	T get(const str& s, const T& dflt = T())
	{
		if(!have(s))
			return dflt;
		T t;
		std::istringstream(props[s][0]) >> std::boolalpha >> t;
		return t;
	}

	/**
	 * Get the native string config variable's value.
	 *
	 * @param s The config variable whose value is sought.
	 * @param dflt A default value to use if the variable was not
	 * found in the config file.
	 *
	 * @return The value that the variable s is set to in the config
	 * file else dflt if not present.
	 */
	str get(const str& s, const str& dflt = "")
	{
		return have(s) ? props[s][0] : dflt;
	}

	/**
	 * Get a file-path converted config variable's value.
	 *
	 * @param s The config variable whose value is sought.
	 * @param dflt A default value to use if the variable was not
	 * found in the config file.
	 *
	 * @return The value that the variable s is set to in the config
	 * file after file glob explnsion has been applied else dflt if
	 * not present.
	 */
	str get_exp(const str& s, const str& dflt = "")
	{
		return have(s) ? expand_env(props[s][0], WRDE_SHOWERR|WRDE_UNDEF) : dflt;
	}

	/**
	 * Get the native string vector config variable's values
	 * when the same variable is provided several times in the
	 * config file.
	 *
	 * @param s The config variable whose values are sought.
	 *
	 * @return All the values that the variable s is set to in the config
	 * file as a vector of strings (str_vec). May be empty if variable is
	 * not present.
	 */
	const str_vec& get_vec(const str& s)
	{
		return props[s];
	}

	/**
	 * Get the native string vector config variable's file-name
	 * exanded values when the same variable is provided several
	 * times in the config file.
	 *
	 * @param s The config variable whose values are sought.
	 *
	 * @return All the file-name expanded  values that the
	 * variable s is set to in the config file as a vector
	 * of strings (str_vec). May be empty if variable is
	 * not present.
	 */
	str_vec get_exp_vec(const str& s)
	{
		str_vec v = get_vec(s);
		for(siz i = 0; i < v.size(); ++i)
			v[i] = expand_env(v[i], WRDE_SHOWERR|WRDE_UNDEF);
		return v;
	}

	/**
	 * Check if the config file has a given variable set.
	 */
	bool has(const str& s)
	{
		return(props.find(s) != props.end() && !props[s].empty());
	}

	/**
	 * Synonym forbool has(const str& s).
	 */
	bool have(const str& s) { return has(s); }

	/**
	 * Send public chat (via rcon) to a given player
	 * with slot number num.
	 *
	 * @param num The slot number of the player you want
	 * to mark chat for.
	 *
	 * @text The message to be printed.
	 *
	 * @return true if rcon succeeded or false if it failed.
	 */
	bool chat_to(slot num, const str& text);

	/**
	 * Send public chat (via rcon) to a given player
	 * with the given GUID.
	 *
	 * @param guid The GUID of the player you want
	 * to mark chat for.
	 *
	 * @text The message to be printed.
	 *
	 * @return true if rcon succeeded or false if it failed.
	 */
	bool chat_to(const GUID& guid, const str& text);

	/**
	 * Send public chat (via rcon) to a given player
	 * with the given name.
	 *
	 * @param name The name of the player you want
	 * to mark chat for.
	 *
	 * @text The message to be printed.
	 *
	 * @return true if rcon succeeded or false if it failed.
	 */
	bool chat_to(const str& name, const str& text);

	/**
	 * Set a variable to be auto-updated from builtin commands. The variable is set to a supplied
	 * default value if a default value can not be found in the config file.
	 * @param plugin pointer to the calling plugin
	 * @param name variable name. A configurable prefix is added to this name for the cvar lookup.
	 * @param var the actual variable to be updated
	 * @param dflt the default value to use if none can be found in the config file.
	 */
	template<typename T>
	void add_var_event(class KatinaPlugin* plugin, const str& name, T& var, const T& dflt = T())
	{
		var = get(name, dflt);
		vars[plugin][name].reset(new cvar_t<T>(var));
		if(logmode > LOG_NORMAL)
			log("CVAR: " << plugin->get_id() << ": " << name << " = " << var);
	}

	struct evtent_hold
	{
		event_t e;
		KatinaPlugin* p;
		str_vec after;
		bool operator==(const evtent_hold& erase) const { return e == erase.e && p == erase.p; }
	};

	TYPEDEF_VEC(evtent_hold, event_hold_vec);
	event_hold_vec erase_events;
	event_hold_vec defer_events;

	void add_log_event(class KatinaPlugin* plugin, event_t e)
	{
		events[e].push_back(plugin);
	}

	void add_log_event(class KatinaPlugin* plugin, event_t e, const str_vec& after)
	{
		if(after.empty())
			add_log_event(plugin, e);

		plugin_vec found;
		plugin_map_citer p;
		for(const str& id: after)
			if((p = plugins.find(id)) != plugins.cend())
				found.push_back(p->second);

		plugin_lst& list = events[e];

		plugin_vec_iter pvi;
		for(plugin_lst_iter pli = list.begin(); pli != list.end(); ++pli)
		{
			if((pvi = std::find(found.begin(), found.end(), *pli)) == found.end())
				continue;

			found.erase(pvi);

			if(found.empty())
				{ list.insert(++pli, plugin); break; }
		}

		if(!found.empty())
			defer_events.push_back({e, plugin, after});
	}

	void del_log_event(class KatinaPlugin* plugin, event_t e)
	{
		plugin_lst_iter i = std::find(events[e].begin(), events[e].end(), plugin);
		if(i != events[e].end())
			erase_events.push_back({e, plugin}); // TODO: clang++ crashes here
	}

	void del_log_events(class KatinaPlugin* plugin)
	{
		for(const event_map_vt& vt: events)
			for(const plugin_lst_vt& p: vt.second)
				if(p == plugin)
					erase_events.push_back({vt.first, p});
	}

	/**
	 * Find out if the player with the GUID is a server admin.
	 *
	 * This function checks the config file for admin.guid: entries
	 * and also the admin.dat config file of the running server if
	 * specified by the admin.dat.file: variable in the config file.
	 */
	bool is_admin(const GUID& guid);

	/**
	 *	Start the logfile scanning.
	 *
     * @param config path to config directory [$HOME/.katina]
     * @return true if terminated normally else false;
     */
	bool start(const str& dir);
};

} // katina

#endif	// _OASTATS_KATINA_H

