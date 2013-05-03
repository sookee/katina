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

namespace oastats {

using namespace oastats::net;
using namespace oastats::types;

class Katina
{
private:
	bool done;
	bool active;

	RCon server;

	typedef std::map<str, str_vec> property_map;
	typedef std::pair<const str, str_vec> property_map_pair;
	typedef property_map::iterator property_map_iter;
	typedef property_map::const_iterator property_map_citer;

	typedef std::pair<property_map_iter, property_map_iter> property_map_iter_pair;
	typedef std::pair<property_map_iter, property_map_iter> property_map_range;

	property_map props;

	plugin_map plugins; // id -> KatinaPlugin*
	str_map plugin_files; // id -> filename (for reloading))

	GUID guid_from_name(const str& name);
	bool extract_name_from_text(const str& line, GUID& guid, str& text);
	bool load_plugin(const str& file);
	bool unload_plugin(const str& id);
	bool reload_plugin(const str& id);

public:
	Katina();

	str mapname;
	siz_guid_map clients; // slot -> GUID
	guid_str_map players; // GUID -> name
	guid_siz_map teams; // GUID -> 'R' | 'B'

	KatinaPlugin* get_plugin(const str& id, const str& version);

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

	/**
	 *
     * @param config path to config directory [$HOME/.katina]
     * @return
     */
	bool start(const str& config);
};

} // oastats

#endif	// _OASTATS_KATINA_H

