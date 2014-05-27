#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_NEXT_MAP_H
#define	_OASTATS_KATINA_PLUGIN_NEXT_MAP_H
/*
 * File:   KatinaPluginNextMap.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 27, 2014, 01:12 AM
 */

#include <map>
#include <utility>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/Database.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

#include <pthread.h>

namespace katina { namespace plugin {

using namespace oastats;
using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

struct vote
{
	str mapname;
	int count;
	vote(): count(0) {}
	vote(const str& mapname, int count): mapname(mapname), count(count) {}
};

typedef std::map<GUID, vote> guid_vote_map;
typedef guid_vote_map::value_type guid_vote_map_pair;
typedef guid_vote_map::iterator guid_vote_map_iter;
typedef guid_vote_map::const_iterator guid_vote_map_citer;

class KatinaPluginNextMap
: public KatinaPlugin
{
private:
//	KatinaPluginVotes* votes;

	str& mapname;
	siz_guid_map& clients; // slot -> GUID
	guid_str_map& players; // GUID -> name
	guid_siz_map& teams; // GUID -> 'R' | 'B'
	RCon& server;

	Database db;

	guid_vote_map votes;

	str_siz_map played; // when was each map last played?

	str rotmap; // next map command on rotation

	bool active;

public:
	KatinaPluginNextMap(Katina& katina);

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	//virtual void cvar_event(const str& name, const str& value);
	
	virtual bool init_game(siz min, siz sec, const str_map& cvars);
	virtual bool client_connect_info(siz min, siz sec, siz num, const GUID& guid, const str& ip);
	virtual bool client_disconnect(siz min, siz sec, siz num);
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
	virtual bool exit(siz min, siz sec);

	virtual void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_NEXT_MAP_H
