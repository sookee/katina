#pragma once
#ifndef _OASTATS_DATABASE_H_
#define _OASTATS_DATABASE_H_

/*
 * Database.h
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include <katina/types.h>
#include <katina/GUID.h>
#include <katina/str.h>
#include <katina/log.h>

#include <mysql.h>

namespace oastats { namespace data {

using namespace oastats;
using namespace oastats::types;
using namespace oastats::string;
using namespace oastats::log;

typedef my_ulonglong game_id;
typedef my_ulonglong row_count;

extern const game_id bad_id;
extern const game_id null_id;

class Database
{
	bool active;

	str host;
	siz port;
	str user;
	str pass;
	str base;

	MYSQL mysql;

public:
	Database();
	~Database();

	void config(const str& host, siz port, const str& user, const str& pass, const str& base)
	{
		this->host = host;
		this->port = port;
		this->user = user;
		this->pass = pass;
		this->base = base;
	}

	void on();

	void off();

	// == DATABASE ==
	//
	//  kills: game_id guid weap count
	// deaths: game_id guid weap count
	//   caps: game_id guid count
	//   time: game_id guid count // seconds in game (player not spec)

	bool escape(const str& from, str& to);

	//   game: game_id host port date map

	game_id add_game(const str& host, const str& port, const str& mapname);

	/**
	 *
	 * @param id
	 * @param table "kills" | "deaths"
	 * @param guid
	 * @param weap
	 * @param count
	 * @return
	 */
	bool add_weaps(game_id id, const str& table, const GUID& guid, siz weap, siz count);

	bool add_caps(game_id id, const GUID& guid, siz count);
	bool add_time(game_id id, const GUID& guid, siz count);

	bool add_player(const GUID& guid, const str& name);

	/**
	 *
	 * @param type
	 * @param item
	 * @param guid
	 * @param count
	 * @return 0 = error, 1 = inserted, 2 = updated
	 */
	row_count add_vote(const str& type, const str& item, const GUID& guid, int count);

	bool add_ovo(game_id id, const GUID& guid1, const GUID& guid2, siz count);

	bool read_map_votes(const str& mapname, guid_int_map& map_votes);

	bool set_preferred_name(const GUID& guid, const str& name);
	bool get_preferred_name(const GUID& guid, str& name);

	bool get_ingame_stats(const GUID& guid, const str& mapname, siz prev, str& stats);
};

}} // oastats::data

#endif /* _OASTATS_DATABASE_H_ */
