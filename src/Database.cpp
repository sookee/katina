/*
 * Database.cpp
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include <katina/Database.h>

#include <mysql.h>

#include <ctime>
#include <katina/log.h>

namespace oastats { namespace data {

const game_id bad_id(-1);
const game_id null_id(0);


Database::Database(): active(false) { mysql_init(&mysql); }
Database::~Database() { off(); }

void Database::on()
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

void Database::off()
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

bool Database::escape(const str& from, str& to)
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

game_id Database::add_game(const str& host, const str& port, const str& mapname)
{
	if(!active)
		return null_id; // inactive

	log("DATABASE: add_game(" << host << ", " << port << ", " << mapname << ")");

	str safe_mapname;
	if(!escape(mapname, safe_mapname))
	{
		log("DATABASE: ERROR: failed to escape: " << mapname);
		return bad_id;
	}

	str sql = "insert into `game`"
		" (`host`, `port`, `map`) values (INET_ATON('"
		+ host + "'),'" + port + "','" + safe_mapname + "')";

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
bool Database::add_weaps(game_id id, const str& table, const GUID& guid, siz weap, siz count)
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

bool Database::add_caps(game_id id, const GUID& guid, siz count)
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

bool Database::add_time(game_id id, const GUID& guid, siz count)
{
	if(!active)
		return true; // not error

	log("DATABASE: add_time(" << id << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `time` (`game_id`, `guid`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << count << "')";

	str sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to add_time; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	return true;
}

bool Database::add_player(const GUID& guid, const str& name)
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

row_count Database::add_vote(const str& type, const str& item, const GUID& guid, int count)
{
	if(!active)
		return true; // not error

	log("DATABASE: add_vote(" << type << ", " << item << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `votes` (`type`,`item`,`guid`,`count`) values ('"
		<< type << "','" << item << "','" << guid << "','" << count << "')"
		<< " on duplicate key update `count` = '" << count << "'";

	str sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to add_vote; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return 0; // error
	}

	return mysql_affected_rows(&mysql);
}

bool Database::add_ovo(game_id id, const GUID& guid1, const GUID& guid2, siz count)
{
	if(!active)
		return true; // not error

	log("DATABASE: add_ovo(" << id << ", " << guid1 << ", " << guid2 << ", " << count << ")");

	soss oss;
	oss << "insert into `ovo` (`game_id`,`guid1`,`guid2`,`count`) values ('"
		<< id << "','" << guid1 << "','" << guid2 << "','" << count << "')";

	str sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to add_ovo; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	return true;
}

bool Database::read_map_votes(const str& mapname, guid_int_map& map_votes)
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

bool Database::set_preferred_name(const GUID& guid, const str& name)
{
	if(!active)
		return true; // not error

	log("DATABASE: set_preferred_name(" << guid << ", " << name << ")");

	str safe_name;
	if(!escape(name, safe_name))
	{
		log("DATABASE: ERROR: failed to escape: " << name);
		return bad_id;
	}

	soss oss;
	oss << "insert into `user` (`guid`,`name`) values ('"
		<< guid << "','" << safe_name << "') on duplicate key update `name` = '" << safe_name << "'";

	str sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to set_preferred_name; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	return true;
}
bool Database::get_preferred_name(const GUID& guid, str& name)
{
	log("DATABASE: get_preferred_name(" << guid << ", " << name << ")");

	soss oss;
	oss << "select name from user where guid = '" << guid << "'";

	str sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to get_preferred_name; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	MYSQL_RES* result = mysql_store_result(&mysql);

	MYSQL_ROW row;
	if((row = mysql_fetch_row(result)))
	{
		name = row[0];
	}
	mysql_free_result(result);
	return true;
}

bool Database::get_ingame_stats(const GUID& guid, const str& mapname, siz prev, str& stats)
{
	log("DATABASE: get_ingame_stats(" << guid << ", " << mapname << ", " << prev << ")");

	std::time_t now = std::time(0);
	std::tm t = *gmtime(&now);

	if(prev > 3)
		return false;

	siz syear = t.tm_year + 1900;
	siz smonth = t.tm_mon; // 0 - 11
	if(smonth < prev)
	{
		smonth = smonth + 12 - prev + 1; // 1 - 12
		--syear;
	}
	else
		smonth = smonth - prev + 1; // 1 - 12

	siz eyear = syear;

	siz emonth = smonth + 1;
	if(emonth > 12)
	{
		emonth = 1;
		++eyear;
	}

	bug_var(syear);
	bug_var(smonth);
	bug_var(eyear);
	bug_var(emonth);

	soss oss;
	oss << "select `game_id` from `game` where `map` = '" << mapname << "'";
	oss << " and `date` >= TIMESTAMP('" << syear << '-' << (smonth < 10 ? "0":"") << smonth << '-' << "01" << "')";
	oss << " and `date` <  TIMESTAMP('" << eyear << '-' << (emonth < 10 ? "0":"") << emonth << '-' << "01" << "')";
	str subsql = oss.str();
	
	oss.clear();
	oss.str("");
	oss << "select sum(`kills`.`count`) from `kills` where `kills`.`guid` = '";
	oss << guid << "'";
	oss << " and `game_id` in (" << subsql << ")"; 
	
	str sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to get_ingame_stats; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	MYSQL_RES* result = mysql_store_result(&mysql);

	MYSQL_ROW row;
	if(!(row = mysql_fetch_row(result)))
	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: fetching row; " << mysql_error(&mysql));
		mysql_free_result(result);
		return false;
	}
	
	str kills = row[0];
	mysql_free_result(result);
	
	oss.clear();
	oss.str("");
	oss << "select sum(`caps`.`count`) from `caps` where `caps`.`guid` = '";
	oss << guid << "'";
	oss << " and `game_id` in (" << subsql << ")"; 
	
	sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to get_ingame_stats; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	result = mysql_store_result(&mysql);

	if(!(row = mysql_fetch_row(result)))
	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: fetching row; " << mysql_error(&mysql));
		mysql_free_result(result);
		return false;
	}
	
	str caps = row[0];
	mysql_free_result(result);
	
	oss.clear();
	oss.str("");
	oss << "select sum(`time`.`count`) from `time` where `time`.`guid` = '";
	oss << guid << "'";
	oss << " and `game_id` in (" << subsql << ")"; 
	
	sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to get_ingame_stats; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	result = mysql_store_result(&mysql);

	if(!(row = mysql_fetch_row(result)))
	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: fetching row; " << mysql_error(&mysql));
		mysql_free_result(result);
		return false;
	}
	
	str secs = row[0];
	mysql_free_result(result);
	
	siz hours = 0;
	siz fph = 0;
	siz cph = 0;
	siss iss(kills + " " + caps + " " + secs); // seconds
	if(!(iss >> fph >> cph >> hours))
	{
		log("DATABASE ERROR: parsing results: " << (kills + " " + caps + " " + secs));
		return false;
	}
	
	stats = "^3FPH^7: ^2unknown ^3CPH^7: ^2unknown";
	
	if(hours)
	{
		hours /= (60 * 60);
		fph /= hours;
		cph /= hours;

		soss oss;
		oss << "^3FPH^7: ^2" << fph << " ^3CPH^7: ^2" << cph;
		stats = oss.str();
	}

	return true;
}

}} // oastats::data
