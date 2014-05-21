
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

using namespace oastats::log;
using namespace oastats::types;
using namespace oastats::string;

KATINA_PLUGIN_TYPE(KatinaPluginPlayerDb);
KATINA_PLUGIN_INFO("katina::player::db", "Katina Player Database", "0.1-dev");

MYSQL mysql;
str host;
siz port = 0;
str user;
str pass;
str base;

struct player_do
{
	GUID guid;
	uint32_t ip;
	str name;

	player_do(): ip(0) {}

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

typedef std::map<siz, uint32_t> ip_map; // num -> ip
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

void db_add(const player_do& p)
{
	bug("PLAYER DB: add: " << p.guid << " " << p.ip << " " << p.name);

	if(p.ip == 0)
	{
		bug("ZERO: p.ip: " << p.ip);
		return;
	}

	if(player_cache.count(p))
	{
		bug("CACHE HIT DATABASE WRITE AVOIDED");
		return;
	}

	std::ostringstream sql;

	sql << "insert into `" << base << "`.`info` values ('";
	sql << p.guid << "'," << p.ip << ",'" << p.name << "')";

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
, mapname(katina.mapname)
, clients(katina.clients)
, players(katina.players)
, teams(katina.teams)
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

bool KatinaPluginPlayerDb::client_connect_info(siz min, siz sec, siz num, const GUID& guid, const str& ip)
{
	if(trim_copy(ip).empty())
	{
		plog("WARN: empty ip address");
		return true;
	}

	struct in_addr ip4;

	if(!inet_pton(AF_INET, trim_copy(ip).c_str(), &ip4) || !ip4.s_addr)
		plog("ERROR: converting IP address: " << ip << " for [" << guid << "]");
	else
		ips[num] = ip4.s_addr;

	return true;
}

bool KatinaPluginPlayerDb::client_disconnect(siz min, siz sec, siz num)
{
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

bool KatinaPluginPlayerDb::client_userinfo_changed(siz min, siz sec, siz num, siz team
		, const GUID& guid, const str& name, siz hc)
{
//	if(!active)
//		return true;
//	plog("client_userinfo_changed(" << num << ", " << team << ", " << guid << ", " << name << ")");
//	plog("clients[" << num << "]         : " << clients[num]);
//	plog("players[clients[" << num << "]]: " << players[clients[num]]);
	if(guid.is_bot())
		return true;

	if(!ips[num])
	{
		plog("ERROR: ip data not known for: " << guid << ", [" << name << "]");
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
