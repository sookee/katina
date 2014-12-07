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
KATINA_PLUGIN_INFO("katina::playerdb", "Katina Player Database", "0.1");

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

//bool query(const str& sql)
//{
//	if(mysql_real_query(&mysql, sql.c_str(), sql.length()) && mysql_errno(&mysql) != ER_DUP_ENTRY)
//	{
//		plog("DATABASE ERROR: [" << mysql_errno(&mysql) << "] " << mysql_error(&mysql));
//		plog("              : sql = " << sql);
//		return false;
//	}
//
//	return true;
//}

///**
// * Perform an "insert" sql statement.
// * @param sql The "insert" sql statement.
// * @return true on success else false
// */
//bool insert(const str& sql) { return query(sql); }
//
//bool insert(const str& sql, my_ulonglong& insert_id)
//{
//	if(!insert(sql))
//		return false;
//
//	insert_id = mysql_insert_id(&mysql);
//
//	return true;
//}
//
//bool db_escape(const str& from, str& to)
//{
//	if(from.size() > 511)
//	{
//		plog("DATABASE ERROR: escape: string too long at line: " << __LINE__);
//		return false;
//	}
//	char buff[1024];
//	to.assign(buff, mysql_real_escape_string(&mysql, buff, from.c_str(), from.size()));
//	return true;
//}

void KatinaPluginPlayerDb::db_add(const player_do& p)
{
	if(player_cache.count(p))
		return;

	str safe_name;

	if(!db.escape(p.name, safe_name))
		return;

	// insert into info values ('XXXXXXXX', INET_ATON('123.123.234.234'), 'testing')

	soss sql;
	sql << "insert into `info` values ('";
	sql << p.guid << "',INET_ATON('" << p.ip << "'),'" << safe_name << "')";

	db.insert(sql.str());
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
}

bool KatinaPluginPlayerDb::open()
{
	str default_db = katina.get("db");

	pbug_var(default_db);

	if(!default_db.empty())
	{
		plog("DEFAULT DB: " << default_db);
		default_db += ".";
	}

	str host = katina.get("playerdb.db.host", katina.get(default_db + "db.host", "localhost"));
	siz port = katina.get("playerdb.db.port", katina.get(default_db + "db.port", 3306));
	str user = katina.get("playerdb.db.user", katina.get(default_db + "db.user", ""));
	str pass = katina.get("playerdb.db.pass", katina.get(default_db + "db.pass", ""));
	str base = katina.get("playerdb.db.base", katina.get(default_db + "db.base"));

	pbug_var(host);
	pbug_var(port);
	pbug_var(user);
	pbug_var(base);

	if(base.empty())
	{
		plog("FATAL: Database config not found");
		return false;
	}

	db.config(host, port, user, pass, base);

	if(!db.check())
	{
		plog("FATAL: Database can not connect");
		return false;
	}

	katina.add_var_event(this, "playerdb.active", active, false);

	katina.add_log_event(this, KE_INIT_GAME);
	katina.add_log_event(this, KE_CLIENT_CONNECT_INFO);
	katina.add_log_event(this, KE_CLIENT_DISCONNECT);
	katina.add_log_event(this, KE_CLIENT_USERINFO_CHANGED);

	return true;
}

str KatinaPluginPlayerDb::get_id() const { return ID; }
str KatinaPluginPlayerDb::get_name() const { return NAME; }
str KatinaPluginPlayerDb::get_version() const { return VERSION; }

TYPEDEF_MAP(slot, GUID, slot_guid_map);
TYPEDEF_MAP(slot, str, slot_str_map);

//static slot_guid_map hold_guids;
//static slot_str_map hold_ips;

str KatinaPluginPlayerDb::api(const str& cmd, void* blob)
{
	siss iss(cmd);

	str call;
	if(!(iss >> call))
		return "ERROR: unknown call: " + cmd;

	if(call == "slot_to_ip")
	{
		slot num;
		if(!(iss >> num))
			return "ERROR: parsing slot: " + cmd;

		if(num == slot::bad)
			return "ERROR: slot number not known: " + str(num);

		GUID guid = katina.getClientGuid(num);

		if(guid == null_guid)
			return "ERROR: guid not known for slot: " + str(num);

		if(ips.find(guid) == ips.end())
			return "ERROR: ip not known for guid: " + str(guid);

		return ips.at(guid);
	}
	else if(call == "guid_to_ip")
	{
		GUID guid;
		if(!(iss >> guid))
			return "ERROR: parsing guid: " + cmd;

		if(ips.find(guid) == ips.end())
			return "ERROR: ip not known for guid: " + str(guid);

		return ips.at(guid);
	}

	return KatinaPlugin::api(cmd);
}

bool KatinaPluginPlayerDb::init_game(siz min, siz sec, const str_map& svars)
{
	if(!active)
		return true;

	if(katina.mod_katina < "0.1.2")
	{
		plog("ERROR: " + ID + " REQUIRES mod_katina >= 0.1.2");
		active = false;
		katina.del_log_events(this);
	}
	return true;
}

bool KatinaPluginPlayerDb::client_connect_info(siz min, siz sec, slot num, const GUID& guid, const str& ip)
{
	if(!active)
		return true;

	if(ip.empty())
	{
		plog("WARN: empty ip address");
		return true;
	}

	if(katina.mod_katina < "0.1.2")
		return true;

	ips[guid] = ip;

	return true;
}

bool KatinaPluginPlayerDb::client_disconnect(siz min, siz sec, slot num)
{
	if(!active)
		return true;

	GUID guid = katina.getClientGuid(num);

	if(guid == null_guid)
		return true;

	player_set_iter i, p;
	for(i = player_cache.begin(); i != player_cache.end();)
	{
		p = i++;
		if(ips.find(guid) != ips.end() && p->ip == ips.at(guid))
			player_cache.erase(p);
	}
	ips.erase(guid);

	return true;
}

bool KatinaPluginPlayerDb::client_userinfo_changed(siz min, siz sec, slot num, siz team
		, const GUID& guid, const str& name, siz hc)
{
	if(!active)
		return true;

	if(guid.is_bot())
		return true;

	if(katina.mod_katina < "0.1.2")
		return true;

	if(ips.find(guid) == ips.end())
	{
		static guid_set guids;
		if(guids.count(guid))
		{
			plog("WARN: ip data not known for: " << guid << ", [" << name << "]");
			guids.insert(guid); // only once
		}
		return true;
	}

	db_add({guid, ips.at(guid), name});

	return true;
}

void KatinaPluginPlayerDb::close()
{
}

}} // katina::plugin
