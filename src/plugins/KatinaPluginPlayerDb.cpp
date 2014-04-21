
#include "KatinaPluginPlayerDb.h"

#include <katina/types.h>
#include <katina/log.h>

#include <string>

#include <arpa/inet.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(KatinaPluginPlayerDb);
KATINA_PLUGIN_INFO("katina::player::db", "Katina Example", "0.1-dev");

struct player_do
{
	GUID guid;
	int32_t ip;
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
typedef std::pair<player_set_iter, bool> player_set_ret;

typedef std::map<siz, player_set> player_map;
typedef player_map::iterator player_map_iter;

player_map players;

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

void db_add(const player_set::value_type& p)
{
	bug("PLAYER DB: add: " << p.guid << " " << p.ip << " " << p.name);
}

str to_string(siz n)
{
	std::ostringstream oss;
	oss << n;
	return oss.str();
}

void parse_namelog(const str& text, siz num)
{
	// ^2|<^8MAD^1Kitty Too^7: ^2why no? ))
	// ^30  (*2BC45233)  77.123.107.231^7 '^2BDSM^7'
	// ^31  (*1DE6454E)  90.192.206.146^7 '^4A^5ngel ^4E^5yes^7'
	// -  (*4A8B117F)    86.1.110.133^7 'Andrius [LTU]^7'
	// ^33  (*F1147474)    37.47.104.24^7 'UFK POLAND^7'
	// ^34  (*ACF58F90)  95.118.224.206^7 '^1Vamp ^3G^1i^3r^1l^7'
	// ^35  (*EAF1A70C)  88.114.147.124^7 ':D^7'
	// -  (*E5DAD0FE)   201.37.205.57^7 '^1*^7M^1*^7^^1p^7ev^7'
	// -  (*B45368DF)    82.50.105.85^7 'Kiloren^7'
	// ^36  (*5F9DFD1F)      86.2.36.24^7 'SodaMan^7'
	// -  (*11045255)   79.154.175.14^7 '^3R^^2ocket^7' '^4G^1O^3O^4G^2L^1E^7'

	std::istringstream iss(text);
	str line, skip, n, id, ip, names;
	while(std::getline(iss, line))
	{
//		bug(line);

		std::istringstream iss(line);
		if(!std::getline(iss >> n >> id >> ip >> std::ws, names) || n == "!namelog:")
			continue;

		if(id[0] != '(' || id [1] != '*' || id.size() != 11 || !is_ip(ip))
			continue;

		if(n == "-")
			continue;

		if(n != to_string(num))
			continue;

		GUID guid = id.substr(2, 8);

//		bug("num  : " << num);
//		bug("guid : " << guid);
//		bug("ip   : " << ip);
//		bug("names: " << names);

		str name;
		iss.clear();
		iss.str(names);
		while(std::getline(iss, skip, '\'') && std::getline(iss, name, '\''))
		{
			player_set& infos = players[num];
			player_set::value_type p;
			p.guid = guid;
			p.name = name;

			struct in_addr ip4;

			if(!inet_pton(AF_INET, ip.c_str(), &ip4))
				plog("");
			else
			{
				p.ip = ip4.s_addr;
				player_set_ret ret = infos.insert(p);
				if(ret.second) // new element inserted
					db_add(p);
			}
		}

		break;
	}
}

void KatinaPluginPlayerDb::add_player(siz num)
{
	str rep;
	server.command("!namelog", rep);
	if(!rep.empty())
		parse_namelog(rep, num);
}

void KatinaPluginPlayerDb::sub_player(siz num)
{
	katina::plugin::players.erase(num);
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
}

bool KatinaPluginPlayerDb::open()
{
	katina.add_var_event(this, "player.db.active", active);
	//katina.add_var_event(this, "flag", "0");
	katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);

	return true;
}

str KatinaPluginPlayerDb::get_id() const { return ID; }
str KatinaPluginPlayerDb::get_name() const { return NAME; }
str KatinaPluginPlayerDb::get_version() const { return VERSION; }

bool KatinaPluginPlayerDb::client_connect(siz min, siz sec, siz num)
{
//	if(!active)
//		return true;
	plog("client_connect(" << num << ")");
	add_player(num);

	return true;
}

bool KatinaPluginPlayerDb::client_disconnect(siz min, siz sec, siz num)
{
//	if(!active)
//		return true;
	plog("client_disconnect(" << num << ")");
	sub_player(num);
	return true;
}

bool KatinaPluginPlayerDb::client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name)
{
//	if(!active)
//		return true;
//	plog("client_userinfo_changed(" << num << ", " << team << ", " << guid << ", " << name << ")");
//	plog("clients[" << num << "]         : " << clients[num]);
//	plog("players[clients[" << num << "]]: " << players[clients[num]]);
	return true;
}

void KatinaPluginPlayerDb::close()
{
}

}} // katina::plugin
