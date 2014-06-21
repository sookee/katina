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

public:
	
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

	bool init_stmt(MYSQL_STMT*& stmt, const str& sql)
	{
		if(!(stmt = mysql_stmt_init(&mysql)))
		{
			log("");
			return false;
		}

		if(mysql_stmt_prepare(stmt, sql.c_str(), sql.size()))
		{
			log("DATABASE ERROR: " << mysql_stmt_error(stmt));
			mysql_stmt_close(stmt);
			stmt = 0;
		}

		return true;
	}

	template<siz SIZE>
	bool init_binds(std::array<MYSQL_BIND, SIZE>& binds)
	{
		memset(binds.data(), 0, binds.size() * sizeof(MYSQL_BIND));
	}

	void bind_param(MYSQL_BIND& bind, siz& s)
	{
		bind.buffer_type = MYSQL_TYPE_LONGLONG;
		bind.buffer = &s;
		bind.is_null = 0;
		bind.length = 0;
		bind.is_unsigned = 1;
	}

	template<siz N>
	void bind_param(MYSQL_BIND& bind, std::array<char, N>& s, siz& len)
	{
		bind.buffer_type = MYSQL_TYPE_VARCHAR;
		bind.buffer = s.data();
		bind.buffer_length = s.size();
		bind.is_null = 0;
		bind.length = &len;
	}

	template<siz SIZE>
	bool bind_stmt(MYSQL_STMT*& stmt, std::array<MYSQL_BIND, SIZE>& binds)
	{
		if(mysql_stmt_bind_param(stmt, binds.data()))
		{
			log("DATABASE ERROR: " << mysql_stmt_error(stmt));
			mysql_stmt_close(stmt);
			stmt = 0;
			return false;
		}
		return true;
	}

	bool kill_stmt(MYSQL_STMT*& stmt)
	{
		int err = mysql_stmt_close(stmt);
		stmt = 0;
		return err == 0;
	}

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

	// Virtual Interface

	/**
	 * Subclasses implement this to deal with things
	 * that need initiaizing every time the database
	 * connection is opened.
	 *
	 * It is called fron Database::on()
	 */
	virtual void init() {}

	virtual void deinit() {}
};

// TODO: put the reference counting in the Database class itself
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
