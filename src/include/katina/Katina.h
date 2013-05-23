#pragma once
#ifndef _OASTATS_KATINA_H
#define	_OASTATS_KATINA_H

/*
 * File:   Katina.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 1, 2013, 6:23 PM
 */

#include "KatinaPlugin.h"
#include "GUID.h"
#include "rcon.h"
#include "types.h"
#include "PKI.h"

#include <list>
#include <pthread.h>

namespace oastats {

using namespace oastats::net;
using namespace oastats::pki;
using namespace oastats::types;

struct cvarevt
{
	str name;
	str value;
	KatinaPlugin* plugin;
	
	bool operator<(const cvarevt& e)
	{
		return &plugin < &e.plugin && name < e.name;
	}
	
	bool operator==(const cvarevt& e)
	{
		return &plugin == &e.plugin && name == e.name;
	}
};

typedef std::list<cvarevt> cvarevt_lst;
typedef std::list<cvarevt>::iterator cvarevt_lst_iter;

enum event_t
{
	INIT_GAME
	, WARMUP
	, CLIENT_CONNECT
	, CLIENT_BEGIN
	, CLIENT_DISCONNECT
	, CLIENT_USERINFO_CHANGED
	, KILL
	, CTF
	, CTF_EXIT
	, SCORE_EXIT
	, AWARD
	, WEAPON_USAGE
	, MOD_DAMAGE
	, PLAYER_STATS
	, SAY
	, SHUTDOWN_GAME
	, EXIT
	, UNKNOWN		
};

typedef std::map<event_t, plugin_vec> event_map;
typedef event_map::iterator event_map_iter;
typedef event_map::const_iterator event_map_citer;

struct cvar
{
	virtual bool get(str& s) const = 0;
	virtual bool set(const str& s) = 0;
};
	
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

typedef std::map<str, cvar*> cvar_map;
typedef cvar_map::iterator cvar_map_iter;
typedef cvar_map::const_iterator cvar_map_citer;

typedef std::map<KatinaPlugin*, cvar_map> cvar_map_map;
typedef cvar_map_map::iterator cvar_map_map_iter;
typedef cvar_map_map::const_iterator cvar_map_map_citer;

inline
std::istream& operator>>(std::istream& is, siz_set& s)
{
	siz v;
	while(is >> v)
		s.insert(v);
	return is;
}

inline
std::ostream& operator<<(std::ostream& os, const siz_set& s)
{
	str sep;
	for(siz_set_citer i = s.begin(); i != s.end(); ++i)
		{ os << sep << *i; sep = " "; }
	return os;
}

class Katina
{
	friend void* cvarpoll(void* vp);
	bool rconset(const str& cvar, str& val);
	
private:
	bool done;
	bool active;

	typedef std::map<str, str_vec> property_map;
	typedef std::pair<const str, str_vec> property_map_pair;
	typedef property_map::iterator property_map_iter;
	typedef property_map::const_iterator property_map_citer;

	typedef std::pair<property_map_iter, property_map_iter> property_map_iter_pair;
	typedef std::pair<property_map_iter, property_map_iter> property_map_range;

	pthread_t cvarevts_thread;
	pthread_mutex_t cvarevts_mtx;

	str prefix;
	cvarevt_lst cvarevts;
	property_map props;

	plugin_map plugins; // id -> KatinaPlugin*
	str_map plugin_files; // id -> filename (for reloading))

	event_map events; // event -> KatinaPlugin*
	cvar_map_map cvars; // plugin* -> {name -> cvar*}
	
	GUID guid_from_name(const str& name);
	bool extract_name_from_text(const str& line, GUID& guid, str& text);
	bool load_config(const str& dir, const str& file, property_map& props);
	bool load_plugin(const str& file);
	bool unload_plugin(const str& id);
	bool reload_plugin(const str& id);

public:
	Katina();
	~Katina();

	// API
	
	PKI pki;
	RCon server;

	str config_dir;
	str mapname;
	siz_guid_map clients; // slot -> GUID
	guid_str_map players; // GUID -> name
	guid_siz_map teams; // GUID -> 'R' | 'B'
	siz logmode;
	std::time_t now;
	
	str get_version();

	KatinaPlugin* get_plugin(const str& id, const str& version);
	
	template<typename Plugin>
	bool get_plugin(const str& id, const str& version, Plugin*& plugin)
	{
		return (plugin = dynamic_cast<Plugin*>(get_plugin(id, version)));
	}

	template<typename T>
	T get(const str& s, const T& dflt = T())
	{
		if(props[s].empty())
			return dflt;
		T t;
		std::istringstream(props[s][0]) >> std::boolalpha >> t;
		return t;
	}

	str get(const str& s, const str& dflt = "")
	{
		return props[s].empty() ? dflt : props[s][0];
	}

	str_vec get_vec(const str& s)
	{
		return props[s];
	}

	bool has(const str& s)
	{
		property_map_range i = props.equal_range(s);
		return i.first != i.second;
	}

	bool have(const str& s) { return has(s); }
	
	bool chat_to(siz num, const str& text);
	bool chat_to(const GUID& guid, const str& text);
	bool chat_to(const str& name, const str& text);

	/**
	 * Set a variable to be auto-updated from a cvar. The variable is set to a supplied
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
		cvars[plugin][name] = new cvar_t<T>(var);
		if(logmode > LOG_NORMAL)
			log("CVAR: " << plugin->get_id() << ": " << name << " = " << var);  
	}

	void add_log_event(class KatinaPlugin* plugin, event_t e)
	{
		events[e].push_back(plugin);
	}
	
	/**
	 *
     * @param config path to config directory [$HOME/.katina]
     * @return
     */
	bool start(const str& dir);
};

} // oastats

#endif	// _OASTATS_KATINA_H

