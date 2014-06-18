//#pragma once
#ifndef _OASTATS_DATABASE_H_
#define _OASTATS_DATABASE_H_
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

#include <array>

#include <mysql.h>

namespace katina { namespace data {

using namespace katina;
using namespace katina::types;
using namespace katina::string;
using namespace katina::log;

typedef my_ulonglong game_id;
typedef my_ulonglong row_count;

extern const game_id bad_id;
extern const game_id null_id;

typedef std::vector<str_vec> str_vec_vec;

// TODO: Make this a superclass and have
// each plugin inherit its own class from it

class Database
{
	friend struct db_scoper;
	friend struct db_transaction_scoper;

	bool active;

	str host;
	siz port;
	str user;
	str pass;
	str base;

	MYSQL mysql;
	
	/**
	 * Open database connection
	 */
	void on();

	/**
	 * Close database connection
	 */
	void off();

	// KatinaPluinStats
	//struct playerstats {};
	MYSQL_STMT *stmt_add_playerstats = 0;
	std::array<MYSQL_BIND, 16> bind_add_playerstats;
	std::array<siz, 15> siz_add_playerstats;
	char guid_add_playerstats[9];
	siz guid_length = 8;

	bool trace = false;

protected:
	
	/**
	 * Perform sql statement.
	 * @param sql The sql statement.
	 * @return true on success else false
	 */
	bool query(const str& sql);

	/**
	 * Perform an "insert" sql statement.
	 * @param sql The "insert" sql statement.
	 * @return true on success else false
	 */
	bool insert(const str& sql) { return query(sql); }
	
	/**
	 * Perform an "insert" sql statement.
	 * @param sql The "insert" sql statement.
	 * @param insert_id is set to the auto increment id (if any)
	 * using mysql_insert_id().
	 * @return true on success else false
	 */
	bool insert(const str& sql, my_ulonglong& insert_id);
	
	/**
	 * Perform an "update" sql statement.
	 * @param sql The "update" sql statement.
	 * @return true on success else false
	 */
	bool update(const str& sql) { return query(sql); }
		
	/**
	 * Perform an "update" or "on duplicate key update" sql statement.
	 * @param sql The "update" or "on duplicate key update" sql statement.
	 * @param update_count is set to the number of rows updated
	 * using mysql_affected_rows(). If the query is an "on duplicate key update"
	 * then update_count will contain: 0 = error, 1 = inserted, 2 = updated
	 * @return true on success else false
	 */
	bool update(const str& sql, my_ulonglong& update_count);

public:
	Database();
	virtual ~Database();

	void config(const str& host, siz port, const str& user, const str& pass, const str& base)
	{
		this->host = host;
		this->port = port;
		this->user = user;
		this->pass = pass;
		this->base = base;
	}

	/**
	 * Ensure connection
	 */
	bool check();

	void set_trace(bool state = true) { trace = state; }

	bool escape(const str& from, str& to);

	str error();
    
	/**
	 * Perform an "select" sql statement.
	 * @param sql The "select" sql statement.
	 * @param rows is a std::vector of std::string std::vectors
	 * containing the returned table in the form rows[row][column].
	 * @return true on success else false
	 */
	bool select(const str& sql, str_vec_vec& rows, siz fields = 0);



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
	
	bool add_weapon_usage(game_id id, const GUID& guid, siz weap, siz shots);
	bool add_mod_damage(game_id id, const GUID& guid, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits);
	bool add_playerstats(game_id id, const GUID& guid,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
		siz carrierFrags, siz carrierFragsRecv);
	bool add_playerstats_ps(game_id id, const GUID& guid,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
		siz carrierFrags, siz carrierFragsRecv);
	bool add_speed(game_id id, const GUID& guid,
			siz dist, siz time, bool has_flag);

	bool read_map_votes(const str& mapname, guid_int_map& map_votes);

	bool set_preferred_name(const GUID& guid, const str& name);
	bool get_preferred_name(const GUID& guid, str& name);

	siz get_kills_per_cap(const str& sql_select_games = "");
	bool get_ingame_boss(const str& mapname, const slot_guid_map& clients, GUID& guid, str& stats);
	bool get_ingame_champ(const str& mapname, GUID& guid, str& stats);
	bool get_ingame_stats(const GUID& guid, const str& mapname, siz prev, str& stats, siz& skill);
	bool get_ingame_crap(const str& mapname, const slot_guid_map& clients, GUID& guid, str& stats);
};

class db_scoper
{
private:
	static siz count;

public:
	Database& db;
	db_scoper(Database& db): db(db)
	{
		bug("db_scoper:init: " << this);
		if(!count++)
		{
			bug("db_scoper:  on: " << this);
			db.on();
		}
	}
	~db_scoper()
	{
		if(!--count)
		{
			db.off();
			bug("db_scoper: off: " << this);
		}
		bug("db_scoper:exit: " << this);
	}
};

class db_transaction_scoper
{
private:
	enum class trans
	{
		ABORT, COMMIT, ROLLBACK
	};

	Database& db;
	bool abort = false;
	trans state = trans::COMMIT;

public:
	db_transaction_scoper(Database& db): db(db)
	{
		bug("db_tx_scoper:  on: " << this);
		db.on();
		if(!db.query("START TRANSACTION"))
		{
			log("DATABASE TRANSACTION ERROR: " << db.error());
			state = trans::ABORT;
			db.off();
		}
	}

	void rollback()
	{
		if(state != trans::ABORT)
			state = trans::ROLLBACK;
	}

	~db_transaction_scoper()
	{
		bool err = false;
		if(state == trans::COMMIT)
			err = db.query("COMMIT");
		else if(state == trans::COMMIT)
			err = db.query("ROLLBACK");

		if(err)
			log("DATABASE TRANSACTION ERROR: " << db.error());

		db.off();
		bug("db_tx_scoper: off: " << this);
	}
};

}} // katina::data

#endif /* _OASTATS_DATABASE_H_ */
