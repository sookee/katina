/*
 * File:   KatinaPluginVotes.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 13, 2013, 10:02 AM
 */

#ifndef KATINA_PLUGIN_VOTES_H
#define	KATINA_PLUGIN_VOTES_H

#include <map>
#include <utility>

#include "KatinaPluginStats.h"
#include <katina/Database.h>
#include <katina/RemoteClient.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

#include <katina/PKI.h>

namespace katina { namespace plugin {

using namespace oastats::pki;
using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

class KatinaPluginVotes
: public KatinaPlugin
{
public:

private:
	Database db;

	// cvars
	bool active;

public:

	/**
	 * The name of the map that map_votes applies to
	 */
	str mapname;
	guid_int_map map_votes; // GUID -> 3

	KatinaPluginVotes(Katina& katina);

	// API
	
	void get_votes(siz& love, siz& hate);

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool init_game(siz min, siz sec, const str_map& cvars);
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
	virtual bool sayteam(siz min, siz sec, const GUID& guid, const str& text);

	virtual void close();
};

}} // katina::plugin

#endif	// KATINA_PLUGIN_VOTES_H

