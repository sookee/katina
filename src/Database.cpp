/*
 * Database.cpp
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
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

#include <katina/Database.h>

#include <mysql.h>

#include <ctime>
#include <cmath>
#include <cstring>

#include <katina/log.h>

namespace katina { namespace data {

siz db_scoper::count = 0;
siz Database::initialized = 0;
std::mutex Database::initialized_mtx;
std::recursive_mutex Database::r_mtx;

Database::Database(): active(false), port(3306)
{
	r_lock_guard syncronized(r_mtx);

	if(!initialized)
	{
		log("DATABASE LIBRARY INIT");
		lock_guard lock(initialized_mtx);
		if(mysql_library_init(0, NULL, NULL))
		{
			log("ERROR: initializing mysql library");
			return;
		}
	}
	++initialized;
	mysql_init(&mysql);
}

Database::~Database()
{
	r_lock_guard syncronized(r_mtx);

	if(!initialized)
		return;
	off();
	if(!--initialized)
	{
		log("DATABASE LIBRARY END");
		lock_guard lock(initialized_mtx);
		mysql_library_end();
	}
}

void Database::on()
{
	r_lock_guard syncronized(r_mtx);

	if(!initialized)
		return;

	if(active)
		return;

	if(!write)
	{
		active = true;
		return;
	}

	if(mysql_real_connect(&mysql, host.c_str(), user.c_str()
		, pass.c_str(), base.c_str(), port, 0, 0) != &mysql)
	{
		log("DATABASE ERROR: Unable to connect: " << mysql_error(&mysql));
		return;
	}

	active = true;

	if(!query("SET NAMES 'utf8', time_zone = '+00:00', TIMESTAMP = " + std::to_string(std::time(0)) + ";"))
	{
		mysql_close(&mysql);
		active = false;
		return;
	}

	init();
}

void Database::off()
{
	r_lock_guard syncronized(r_mtx);

	if(!active)
		return;

	active = false;

	if(!write)
		return;

	deinit();

	mysql_close(&mysql);
}

bool Database::check()
{
	r_lock_guard syncronized(r_mtx);

	if(!active)
		return true;
	if(!write)
		return true;

	const bool was_active = active;

	if(!was_active)
		on();

	bool alive = !mysql_ping(&mysql);

	if(!alive)
		log("DATABASE ERROR: " << mysql_error(&mysql));

	if(!was_active)
		off();

	return alive;
}

// == DATABASE ==
//
//  kills: game_id guid weap count
// deaths: game_id guid weap count
//   caps: game_id guid count
//   time: game_id guid count // seconds in game (player not spec)

bool Database::escape(const str& from, str& to)
{
	r_lock_guard syncronized(r_mtx);

	if(from.size() > 511)
	{
		log("DATABASE ERROR: escape: string too long at line: " << __LINE__);
		return false;
	}
	char buff[1024];
	to.assign(buff, mysql_real_escape_string(&mysql, buff, from.c_str(), from.size()));
	return true;
}

str Database::error()
{
	r_lock_guard syncronized(r_mtx);

	if(!active)
		return "";
	if(!write)
		return "";
	return mysql_error(&mysql);
}

bool Database::query(const str& sql)
{
	r_lock_guard syncronized(r_mtx);

	if(!active)
		return true;
	if(!write)
		return true;

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	return true;
}

bool Database::insert(const str& sql, my_ulonglong& insert_id)
{
	r_lock_guard syncronized(r_mtx);

	insert_id = my_ulonglong(-1);
	if(!active)
		return true;
	if(!write)
		return true;

	if(!insert(sql))
		return false;

	insert_id = mysql_insert_id(&mysql);

	return true;
}

bool Database::update(const str& sql, my_ulonglong& update_count)
{
	r_lock_guard syncronized(r_mtx);

	update_count = 0;
	if(!active)
		return true;
	if(!write)
		return true;

	if(!update(sql))
		return false;

	update_count = mysql_affected_rows(&mysql);

	return true;
}

bool Database::select(const str& sql, str_vec_vec& rows, siz fields)
{
	r_lock_guard syncronized(r_mtx);

	rows.clear();
	if(!active)
		return true;
	if(!write)
		return true;

	if(!query(sql))
		return false;

	MYSQL_RES* result = 0;
	if(!(result = mysql_store_result(&mysql)))
	{
		log("DATABASE ERROR: result: " << mysql_error(&mysql));
		return false;
	}

	if(fields == 0)
		fields = mysql_num_fields(result);

	if(fields != mysql_num_fields(result))
		log("DATABASE: WARNING: parameter fields different from table");

	MYSQL_ROW row;
	while((row = mysql_fetch_row(result)))
	{
		str_vec v(fields);
		for(siz f = 0; f < fields; ++f)
			if(row[f])
				v[f] = row[f];
		rows.push_back(v);
	}

	mysql_free_result(result);
	return true;
}

bool Database::transaction()
{
	r_lock_guard syncronized(r_mtx);

	if(!active)
		return true;
	if(!write)
		return true;

	return query("START TRANSACTION");
}

bool Database::rollback()
{
	r_lock_guard syncronized(r_mtx);

	if(!active)
		return true;
	if(!write)
		return true;

	return !mysql_rollback(&mysql);
}

bool Database::commit()
{
	r_lock_guard syncronized(r_mtx);

	if(!active)
		return true;
	if(!write)
		return true;

	return !mysql_commit(&mysql);
}

}} // katina::data
