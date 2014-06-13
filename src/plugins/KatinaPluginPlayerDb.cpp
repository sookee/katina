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

#include "KatinaPluginPlayerDb.h"

#include <katina/types.h>
#include <katina/str.h>
#include <katina/log.h>

#include <string>
#include <algorithm>

#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>

namespace katina { namespace plugin {

using namespace katina::log;
using namespace katina::types;
using namespace katina::string;

KATINA_PLUGIN_TYPE(KatinaPluginPlayerDb);
KATINA_PLUGIN_INFO("katina::playerdb", "Katina Player Database", "0.1-dev");

MYSQL mysql;
str host;
siz port = 0;
str user;
str pass;
str base;

struct player_do
{
	GUID guid;
	str ip;
	str name;

//	player_do(): ip(0) {}

	bool operator<(const player_do& p) const
	{
		if(guid != p.guid)
			return guid < p.guid;
		if(ip != p.ip)
			return ip < p.ip;
		if(name != p.name)
			return name < p.name;
		return false;
	}
};

typedef std::set<player_do> player_set;
typedef player_set::iterator player_set_iter;
//typedef std::pair<player_set_iter, bool> player_set_ret;

player_set player_cache;

typedef std::map<slot, str> ip_map; // slot -> ip
ip_map ips;

bool is_ip(const str& s)
{
	// \d{1-3}.\d{1-3}.\d{1-3}.\d{1-3}
	siz dot = std::count(s.begin(), s.end(), '.');
	if(dot != 3)
		return false;

	siz dig = std::count_if(s.begin(), s.end(), std::ptr_fun<int, int>(isdigit));
	if(dig > 12)
		return false;

	return s.size() == dot + dig;
}

bool query(const str& sql)
{
	if(mysql_real_query(&mysql, sql.c_str(), sql.length()) && mysql_errno(&mysql) != ER_DUP_ENTRY)
	{
		plog("DATABASE ERROR: [" << mysql_errno(&mysql) << "] " << mysql_error(&mysql));
		plog("              : sql = " << sql);
		return false;
	}

	return true;
}

/**
 * Perform an "insert" sql statement.
 * @param sql The "insert" sql statement.
 * @return true on success else false
 */
bool insert(const str& sql) { return query(sql); }

bool insert(const str& sql, my_ulonglong& insert_id)
{
	if(!insert(sql))
		return false;

	insert_id = mysql_insert_id(&mysql);

	return true;
}

bool db_escape(const str& from, str& to)
{
	if(from.size() > 511)
	{
		plog("DATABASE ERROR: escape: string too long at line: " << __LINE__);
		return false;
	}
	char buff[1024];
	to.assign(buff, mysql_real_escape_string(&mysql, buff, from.c_str(), from.size()));
	return true;
}

void db_add(const player_do& p)
{
	if(player_cache.count(p))
		return;

	str safe_name;

	if(!db_escape(p.name, safe_name))
		return;

	// insert into info values ('XXXXXXXX', INET_ATON('123.123.234.234'), 'testing')

	soss sql;
	sql << "insert into `" << base << "`.`info` values ('";
	sql << p.guid << "',INET_ATON('" << p.ip << "'),'" << safe_name << "')";

	insert(sql.str());
	player_cache.insert(p);
}

str to_string(siz n)
{
	std::ostringstream oss;
	oss << n;
	return oss.str();
}

KatinaPluginPlayerDb::KatinaPluginPlayerDb(Katina& katina)
: KatinaPlugin(katina)
, server(katina.server)
, mapname(katina.get_mapname())
, clients(katina.getClients())
, players(katina.getPlayers())
, teams(katina.getTeams())
, active(true)
{
	mysql_init(&mysql);
}

bool KatinaPluginPlayerDb::open()
{
	//katina.add_var_event(this, "player.db.active", active);
	//katina.add_var_event(this, "flag", "0");
	katina.add_log_event(this, CLIENT_CONNECT_INFO);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);

	host = katina.get("player.db.host", "localhost");
	port = katina.get("player.db.port", 3306);
	user = katina.get("player.db.user");
	pass = katina.get("player.db.pass", "");
	base = katina.get("player.db.base");

	if(mysql_real_connect(&mysql, host.c_str(), user.c_str()
		, pass.c_str(), base.c_str(), port, NULL, 0) != &mysql)
	{
		log("DATABASE ERROR: Unable to connect; " << mysql_error(&mysql));
		return false;
	}
	plog("PLAYER DB DATABASE: on");

	return true;
}

str KatinaPluginPlayerDb::get_id() const { return ID; }
str KatinaPluginPlayerDb::get_name() const { return NAME; }
str KatinaPluginPlayerDb::get_version() const { return VERSION; }

TYPEDEF_MAP(slot, GUID, slot_guid_map);
TYPEDEF_MAP(slot, str, slot_str_map);

static slot_guid_map hold_guids;
static slot_str_map hold_ips;

bool KatinaPluginPlayerDb::client_connect_info(siz min, siz sec, slot num, const GUID& guid, const str& ip)
{
	if(ip.empty())
	{
		plog("WARN: empty ip address");
		return true;
	}

	if(katina.mod_katina < "0.1.1")
	{
		// untrustworthy until NEXT client_userinfo_changed when it can be checked
		plog("PLAYERDB: Holding guid & ip: " << str(num) << " " << str(guid) << ", " << ip << " {" << katina.get_line_number() << "}");
		hold_guids[num] = guid;
		hold_ips[num] = ip;
		return true;
	}

	ips[num] = ip;

	return true;
}

bool KatinaPluginPlayerDb::client_disconnect(siz min, siz sec, slot num)
{
	hold_ips[num].clear();

	player_set_iter i, p;
	for(i = player_cache.begin(); i != player_cache.end();)
	{
		p = i++;
		if(p->ip == ips[num])
			player_cache.erase(p);
	}
	ips.erase(num);

	return true;
}

bool KatinaPluginPlayerDb::client_userinfo_changed(siz min, siz sec, slot num, siz team
		, const GUID& guid, const str& name, siz hc)
{
	if(guid.is_bot())
		return true;

	if(!hold_ips[num].empty())
	{
		str ip = hold_ips[num];
		hold_ips[num].clear();
		if(guid != hold_guids[num]) // then we can't trust the ip
		{
			pbug_var(guid);
			pbug_var(hold_guids[num]);
			plog("PLAYERDB: Unreliable GUID & ip, rejecting: " << str(num) << " " << str(hold_guids[num]) << " " << ip << " {" << katina.get_line_number() << "}");
			return true;
		}
		plog("PLAYERDB: RELIABLE GUID, USING IP: " << str(num) << " " << str(hold_guids[num]) << " " << ip << " {" << katina.get_line_number() << "}");
		ips[num] = ip;
	}

	if(ips.find(num) == ips.end())
	{
		static guid_set guids;
		if(guids.count(guid))
		{
			plog("WARN: ip data not known for: " << guid << ", [" << name << "]");
			guids.insert(guid); // only once
		}
		return true;
	}

	player_do p;
	p.guid = guid;
	p.name = name;
	p.ip = ips[num];

	db_add(p);

	return true;
}

void KatinaPluginPlayerDb::close()
{
	mysql_close(&mysql);
}

}} // katina::plugin
