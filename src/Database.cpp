/*
 * Database.cpp
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include <katina/Database.h>

#include <mysql.h>

#include <ctime>
#include <cmath>

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

bool Database::query(const str& sql)
{
	if(!active)
		return false;

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
	if(!insert(sql))
		return false;
	
	insert_id = mysql_insert_id(&mysql);
	
	return true;
}

bool Database::update(const str& sql, my_ulonglong& update_count)
{
	if(!update(sql))
		return false;
	
	update_count = mysql_affected_rows(&mysql);
	
	return true;
}

bool Database::select(const str& sql, str_vec_vec& rows, siz fields)
{
	if(!query(sql))
		return false;

	MYSQL_RES* result = 0;
	if(!(result = mysql_store_result(&mysql)))
	{
		log("DATABASE ERROR: result: " << mysql_error(&mysql));
		return false;
	}		

	if(fields == 0)
		fields = fields = mysql_num_fields(result);
	
	if(fields != mysql_num_fields(result))
		log("DATABASE: WARNING: parameter fields different from table");

	rows.clear();
	
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

//   game: game_id host port date map

game_id Database::add_game(const str& host, const str& port, const str& mapname)
{
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

	game_id id;
	if(!update(sql, id))
		return bad_id;

	return id;
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
	log("DATABASE: add_weaps(" << id << ", " << table << ", " << guid << ", " << weap << ", " << count << ")");

	soss oss;
	oss << "insert into `" << table << "` (`game_id`, `guid`, `weap`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << weap << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool Database::add_caps(game_id id, const GUID& guid, siz count)
{
	log("DATABASE: add_caps(" << id << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `caps` (`game_id`, `guid`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool Database::add_time(game_id id, const GUID& guid, siz count)
{
	log("DATABASE: add_time(" << id << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `time` (`game_id`, `guid`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool Database::add_player(const GUID& guid, const str& name)
{
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

	return insert(sql);
}

row_count Database::add_vote(const str& type, const str& item, const GUID& guid, int count)
{
	log("DATABASE: add_vote(" << type << ", " << item << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `votes` (`type`,`item`,`guid`,`count`) values ('"
		<< type << "','" << item << "','" << guid << "','" << count << "')"
		<< " on duplicate key update `count` = '" << count << "'";

	str sql = oss.str();
	
	my_ulonglong update_count = 0;
	
	if(!update(sql, update_count))
		return 0;

	return update_count;
}

bool Database::add_ovo(game_id id, const GUID& guid1, const GUID& guid2, siz count)
{
	log("DATABASE: add_ovo(" << id << ", " << guid1 << ", " << guid2 << ", " << count << ")");

	soss oss;
	oss << "insert into `ovo` (`game_id`,`guid1`,`guid2`,`count`) values ('"
		<< id << "','" << guid1 << "','" << guid2 << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool Database::read_map_votes(const str& mapname, guid_int_map& map_votes)
{
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

	str_vec_vec rows;
	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		log("DATABASE: restoring vote: " << rows[i][0] << ": " << rows[i][1]);
		map_votes[GUID(rows[i][0])] = to<int>(rows[i][1]);
	}

	return true;
}

bool Database::set_preferred_name(const GUID& guid, const str& name)
{
	log("DATABASE: set_preferred_name(" << guid << ", " << name << ")");

	str safe_name;
	if(!escape(name, safe_name))
		return bad_id;

	soss oss;
	oss << "insert into `user` (`guid`,`name`) values ('"
		<< guid << "','" << safe_name << "') on duplicate key update `name` = '" << safe_name << "'";

	str sql = oss.str();

	return insert(sql);
}

bool Database::get_preferred_name(const GUID& guid, str& name)
{
	log("DATABASE: get_preferred_name(" << guid << ", " << name << ")");

	soss oss;
	oss << "select name from user where guid = '" << guid << "'";

	str sql = oss.str();

	str_vec_vec rows;
	if(!select(sql, rows, 1))
		return false;

	if(rows.empty())
		return false;

	name = rows[0][0];

	return true;
}

bool calc_period(siz& syear, siz& smonth, siz& eyear, siz& emonth, siz prev = 0)
{
	if(prev > 3)
		return false;

	std::time_t now = std::time(0);
	std::tm t = *gmtime(&now);

	syear = t.tm_year + 1900;
	smonth = t.tm_mon; // 0 - 11
	if(smonth < prev)
	{
		smonth = smonth + 12 - prev + 1; // 1 - 12
		--syear;
	}
	else
		smonth = smonth - prev + 1; // 1 - 12

	eyear = syear;

	emonth = smonth + 1;
	if(emonth > 12)
	{
		emonth = 1;
		++eyear;
	}

	bug_var(syear);
	bug_var(smonth);
	bug_var(eyear);
	bug_var(emonth);
	
	return true;
}

bool Database::get_ingame_champ(const str& mapname, GUID& guid, str& stats)
{
	return true;
}

struct stat_c
{
	siz kills;
	siz caps;
	siz secs;
	siz fph;
	siz cph;
	double idx;
	stat_c(): kills(0), caps(0), secs(0), idx(0.0) {}
};

typedef std::map<str, stat_c> stat_map; // guid -> stat_c
typedef stat_map::iterator stat_map_iter;
typedef stat_map::const_iterator stat_map_citer;
	
bool Database::get_ingame_boss(const str& mapname, const siz_guid_map& clients, GUID& guid, str& stats)
{
	log("DATABASE: get_ingame_boss(" << mapname << ", " << clients.size() << ")");
	siz syear = 0;
	siz smonth = 0;
	siz eyear = 0;
	siz emonth = 0;

	if(!calc_period(syear, smonth, eyear, emonth))
		return false;
	
	stat_map stat_cs;
	str_set guids;
	
	soss oss;
	oss << "select `game_id` from `game` where `map` = '" << mapname << "'";
	oss << " and `date` >= TIMESTAMP('" << syear << '-' << (smonth < 10 ? "0":"") << smonth << '-' << "01" << "')";
	oss << " and `date` <  TIMESTAMP('" << eyear << '-' << (emonth < 10 ? "0":"") << emonth << '-' << "01" << "')";
	str subsql = oss.str();
	
	// select distinct `guid`,sum(`kills`.`count`) from `kills`
	// where `kills`.`guid` in ('F8247501','152299FD','E6686040')
	// group by `guid` order by sum(`kills`.`count`) desc;
	
	str sep;
	oss.clear();
	oss.str("");
	for(siz_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
		if(!i->second.is_bot())
			{ oss << sep << "'" << i->second << "'"; sep = ",";}
	str insql = oss.str();
	
	guid = null_guid;
	stats = "^3FPH^7: ^20 ^3CPH^7: ^20 ^3index^7: ^20.00";

	if(insql.empty())
		return true;
	
	oss.clear();
	oss.str("");
	oss << "select distinct `guid`,sum(`kills`.`count`) from `kills` where `kills`.`guid` in (" << insql << ")";
	oss << " and `game_id` in (" << subsql << ") group by `guid` order by sum(`kills`.`count`) desc"; 
	
	str sql = oss.str();
	
	bug_var(sql);

	str_vec_vec rows;
	
	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(rows[i][0].empty() || rows[i][1].empty())
			continue;
		stat_cs[rows[i][0]].kills = to<siz>(rows[i][1]);
		guids.insert(rows[i][0]);
	}
	
	oss.clear();
	oss.str("");
	oss << "select distinct `guid`,sum(`caps`.`count`) from `caps` where `caps`.`guid` in (" << insql << ")";
	oss << " and `game_id` in (" << subsql << ") group by `guid` order by sum(`caps`.`count`) desc"; 
	
	sql = oss.str();
	
	bug_var(sql);

	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(rows[i][0].empty() || rows[i][1].empty())
			continue;
		stat_cs[rows[i][0]].caps = to<siz>(rows[i][1]);
		guids.insert(rows[i][0]);
	}
	
	oss.clear();
	oss.str("");
	oss << "select distinct `guid`,sum(`time`.`count`) from `time` where `time`.`guid` in (" << insql << ")";
	oss << " and `game_id` in (" << subsql << ") group by `guid` order by sum(`time`.`count`) desc"; 
	
	sql = oss.str();
	
	bug_var(sql);

	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(rows[i][0].empty() || rows[i][1].empty())
			continue;
		stat_cs[rows[i][0]].secs = to<siz>(rows[i][1]);
		guids.insert(rows[i][0]);
	}
	
	if(guids.empty())
		return true;
	
	// -- get ratio of frags to caps
	
	oss.clear();
	oss.str("");
	oss << "select sum(`kills`.`count`) from `kills` where";
	oss << " `kills`.`game_id` in (" << subsql << ")"; 
	
	sql = oss.str();
	
	bug_var(sql);

	if(!select(sql, rows, 1))
		return false;

	double k = 0.0;
	
	if(!rows.empty() && !rows[0].empty())
		k = to<double>(rows[0][0]);
	
	oss.clear();
	oss.str("");
	oss << "select sum(`caps`.`count`) from `caps` where";
	oss << " `caps`.`game_id` in (" << subsql << ")"; 
	
	sql = oss.str();
	
	bug_var(sql);

	if(!select(sql, rows, 1))
		return false;

	double c = 0.0;
	
	if(!rows.empty() && !rows[0].empty())
		c = to<double>(rows[0][0]);
	
	double kpc = c > 0.001 ? (k / c) : 1.0;
	
	bug_var(k);
	bug_var(c);
	bug_var(kpc);
	
	// --
	
	str_set_iter maxi = guids.end();
	double maxv = 0.0;
	
	for(str_set_iter g = guids.begin(); g != guids.end(); ++g)
	{
		if(stat_cs[*g].secs)
		{
			stat_cs[*g].fph = stat_cs[*g].kills * 60 * 60 / stat_cs[*g].secs;
			stat_cs[*g].cph = stat_cs[*g].caps * 60 * 60 / stat_cs[*g].secs;
			stat_cs[*g].idx = std::sqrt(std::pow(stat_cs[*g].fph, 2)
				+ std::pow(stat_cs[*g].cph * kpc, 2));
			if(stat_cs[*g].idx > maxv)
			{
				maxv = stat_cs[*g].idx;
				maxi = g;
			}
		}
	}
	
	if(maxi != guids.end())
	{
		guid = GUID(*maxi);
		if(!guid.is_bot())
		{
			soss oss;
			oss << "^3FPH^7: ^2" << stat_cs[*maxi].fph << " ^3CPH^7: ^2" << stat_cs[*maxi].cph;
			oss << std::fixed;
			oss.precision(2);
			oss << " ^3index^7: ^2" << stat_cs[*maxi].idx;
			stats = oss.str();
		}
	}
	
	return true;
}

bool Database::get_ingame_stats(const GUID& guid, const str& mapname, siz prev, str& stats)
{
	log("DATABASE: get_ingame_stats(" << guid << ", " << mapname << ", " << prev << ")");

	siz syear = 0;
	siz smonth = 0;
	siz eyear = 0;
	siz emonth = 0;

	if(!calc_period(syear, smonth, eyear, emonth, prev))
		return false;
	
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
	
	bug_var(sql);

	str_vec_vec rows;
	
	if(!select(sql, rows, 1))
		return false;
		
	str kills = rows.empty() || rows[0][0].empty() ? "0" : rows[0][0];
	bug_var(kills);
	
	oss.clear();
	oss.str("");
	oss << "select sum(`caps`.`count`) from `caps` where `caps`.`guid` = '";
	oss << guid << "'";
	oss << " and `game_id` in (" << subsql << ")"; 
	
	sql = oss.str();
	bug_var(sql);

	if(!select(sql, rows, 1))
		return false;
		
	str caps = rows.empty() || rows[0][0].empty() ? "0" : rows[0][0];
	bug_var(caps);
	
	oss.clear();
	oss.str("");
	oss << "select sum(`time`.`count`) from `time` where `time`.`guid` = '";
	oss << guid << "'";
	oss << " and `game_id` in (" << subsql << ")"; 
	
	sql = oss.str();
	bug_var(sql);

	if(!select(sql, rows, 1))
		return false;
		
	str secs = rows.empty() || rows[0][0].empty() ? "0" : rows[0][0];
	bug_var(secs);
	
	siz hours = 0;
	siz fph = 0;
	siz cph = 0;
	siss iss(kills + " " + caps + " " + secs);
	
	if(!(iss >> fph >> cph >> hours))
	{
		log("DATABASE ERROR: parsing results: " << (kills + " " + caps + " " + secs));
		return false;
	}

	bug_var(hours);
	bug_var(fph);
	bug_var(cph);

	stats = "^3FPH^7: ^20 ^3CPH^7: ^20";
	
	//hours /= (60 * 60);
	
	if(hours)
	{
		fph = (fph * 60 * 60) / hours;
		cph = (cph * 60 * 60) / hours;

		bug_var(hours);
		bug_var(fph);
		bug_var(cph);

		soss oss;
		oss << "^3FPH^7: ^2" << fph << " ^3CPH^7: ^2" << cph;
		stats = oss.str();
	}

	return true;
}

}} // oastats::data
