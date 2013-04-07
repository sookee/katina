#pragma once
#ifndef _OASTATS_DATABASE_H_
#define _OASTATS_DATABASE_H_

/*
 * Database.h
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include "types.h"
#include "GUID.h"
#include "str.h"
#include "log.h"

#include <mysql.h>
#include <arpa/inet.h>

namespace oastats { namespace data {

using namespace oastats;
using namespace oastats::types;
using namespace oastats::string;
using namespace oastats::log;

typedef my_ulonglong game_id;

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
	Database(): active(false) { mysql_init(&mysql); }
	~Database() { off(); }

	void config(const str& host, siz port, const str& user, const str& pass, const str& base)
	{
		this->host = host;
		this->port = port;
		this->user = user;
		this->pass = pass;
		this->base = base;
	}

	void on()
	{
		if(active)
			return;
		if(mysql_real_connect(&mysql, host.c_str(), user.c_str()
			, pass.c_str(), base.c_str(), port, NULL, 0) != &mysql)
		{
			log("DATABASE ERROR: Unable to connect; " << mysql_error(&mysql));
			return;
		}
		log("DATABASE: on");
		active = true;
	}

	void off()
	{
		if(!active)
			return;
		active = false;
		mysql_close(&mysql);
		log("DATABASE: off");
	}

	// == DATABASE ==
	//
	//  kills: game_id guid weap count
	// deaths: game_id guid weap count
	//   caps: game_id guid count
	//   time: game_id guid count // seconds in game (player not spec)

	bool escape(const str& from, str& to)
	{
		if(from.size() > 511)
		{
			log("ERROR: escape: string too long at line: " << __LINE__);
			return false;
		}
		char buff[1024];
		to.assign(buff, mysql_real_escape_string(&mysql, buff, from.c_str(), from.size()));
		return true;
	}
	//   game: game_id host port date map

	game_id add_game(const str& host, const str& port, const str& mapname)
	{
		if(!active)
			return null_id; // inactive

		in_addr addr;
		if(!inet_aton(host.c_str(), &addr))
		{
			log("DATABASE: ERROR: bad IP address: " << host);
			return bad_id;
		}

		log("DATABASE: add_game(" << to_string(addr.s_addr) << ", " << port << ", " << mapname << ")");

		str safe_mapname;
		if(!escape(mapname, safe_mapname))
		{
			log("DATABASE: ERROR: failed to escape: " << mapname);
			return bad_id;
		}

		str sql = "insert into `game`"
			" (`host`, `port`, `map`) values ('"
			+ to_string(addr.s_addr) + "','" + port + "','" + safe_mapname + "')";

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_mame; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return bad_id;
		}

		return mysql_insert_id(&mysql);
	}

	/**
	 *
	 * @param id
	 * @param table "kills" | "deaths"
	 * @param guid
	 * @param weap
	 * @param count
	 * @return
	 */
	bool add_weaps(game_id id, const str& table, const GUID& guid, siz weap, siz count)
	{
		if(!active)
			return true; // not error

		log("DATABASE: add_weaps(" << id << ", " << table << ", " << guid << ", " << weap << ", " << count << ")");

		soss oss;
		oss << "insert into `" << table << "` (`game_id`, `guid`, `weap`, `count`) values (";
		oss << "'" << id << "','" << guid << "','" << weap << "','" << count << "')";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_weaps; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		return true;
	}

	bool add_caps(game_id id, const GUID& guid, siz count)
	{
		if(!active)
			return true; // not error

		log("DATABASE: add_caps(" << id << ", " << guid << ", " << count << ")");

		soss oss;
		oss << "insert into `caps` (`game_id`, `guid`, `count`) values (";
		oss << "'" << id << "','" << guid << "','" << count << "')";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_caps; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		return true;
	}

	bool add_player(const GUID& guid, const str& name)
	{
		if(!active)
			return true; // not error

		log("DATABASE: add_player(" << guid << ", " << name << ")");

		str safe_name;
		if(!escape(name, safe_name))
		{
			log("DATABASE: ERROR: failed to escape: " << name);
			return bad_id;
		}

		soss oss;
		oss << "insert into `player` (`guid`,`name`) values ('" << guid << "','" << safe_name
			<< "') ON DUPLICATE KEY UPDATE count = count + 1";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_player; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		return true;
	}

	bool add_vote(const str& type, const str& item, const GUID& guid, int count)
	{
		if(!active)
			return true; // not error

		log("DATABASE: add_vote(" << type << ", " << item << ", " << guid << ", " << count << ")");

		soss oss;
		oss << "insert into `votes` (`type`,`item`,`guid`,`count`) values ('"
			<< type << "','" << item << "','" << guid << "','" << count << "')";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_vote; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		return true;
	}

	bool read_map_votes(const str& mapname, guid_int_map& map_votes)
	{
		if(!active)
			return true; // not error

		log("DATABASE: read_recs()");

		str safe_mapname;
		if(!escape(mapname, safe_mapname))
		{
			log("DATABASE: ERROR: failed to escape: " << mapname);
			return bad_id;
		}

		soss oss;
		oss << "select `guid`,`count` from `votes` where `type` = 'map' and `item` = '" << safe_mapname << "'";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to read_recs; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		MYSQL_RES* result = mysql_store_result(&mysql);

		MYSQL_ROW row;
		while((row = mysql_fetch_row(result)))
		{
			log("DATABASE: restoring vote: " << row[0] << ": " << row[1]);
			map_votes[GUID(row[0])] = to<int>(row[1]);
		}
		mysql_free_result(result);
		return true;
	}
};

}} // oastats::data

#endif /* _OASTATS_DATABASE_H_ */
