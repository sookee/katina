
#include <katina/KatinaPlugin.h>
#include "KatinaPluginStats.h"

#include <katina/Database.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(KatinaPluginStats);
KATINA_PLUGIN_INFO("katina::stats", "katina Stats", "0.1-dev");

const siz TEAM_U = 0;
const siz TEAM_R = 1;
const siz TEAM_B = 2;
const siz TEAM_S = 3;

siz map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.end() ? 0 : m.at(key);
}

KatinaPluginStats::KatinaPluginStats(Katina& katina)
: KatinaPlugin(katina)
, active(true)
, write(true)
, in_game(false)
{
}

bool KatinaPluginStats::open()
{
	host = katina.get("rcon.host", "localhost");
	port = katina.get("rcon.port", "27960");
	db.config(
		katina.get("db.host", "localhost")
		, katina.get("db.port", 3306)
		, katina.get("db.user")
		, katina.get("db.pass", "")
		, katina.get("db.base"));
	
	if(!katina.has("db.base") || !katina.has("db.user"))
	{
		plog("FATAL: no database config found");
		return false;
	}
	
	katina.add_var_event(this, "stats_active", active, false);
	katina.add_var_event(this, "stats_write", write, false);
	
	katina.add_log_event(this, EXIT);
	katina.add_log_event(this, SHUTDOWN_GAME);
	katina.add_log_event(this, WARMUP);
	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);
	katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, KILL);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, AWARD);
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, WEAPON_USAGE);
	katina.add_log_event(this, MOD_DAMAGE);
	katina.add_log_event(this, PLAYER_STATS);
	
	return true;
}

str KatinaPluginStats::get_id() const
{
	return ID;
}

str KatinaPluginStats::get_name() const
{
	return NAME;
}

str KatinaPluginStats::get_version() const
{
	return VERSION;
}

bool KatinaPluginStats::exit(siz min, siz sec)
{
	// bug("in_game: " << in_game);
	if(!in_game)
		return true;
	in_game = false;
	if(!active)
		return true;

	// in game timing
	for(guid_stat_iter i = stats.begin(); i != stats.end(); ++i)
	{
		bug("TIMER:         EOG: " << i->first);
		if(i->second.joined_time);
		{
			std::time_t now = std::time(0);
			if(i->second.joined_time)
				i->second.logged_time += now - i->second.joined_time;
			i->second.joined_time = 0;
		}
	}
	
	if(write)
		db.on();
	
	game_id id = db.add_game(host, port, katina.mapname);

	if(id != null_id && id != bad_id)
	{
		// TODO: insert game stats here
		for(guid_stat_citer p = stats.begin(); p != stats.end(); ++p)
		{
			//const str& player = katina.players.at(p->first);

			siz count;
			for(std::set<siz>::iterator weap = db_weaps.begin(); weap != db_weaps.end(); ++weap)
			{
				if((count = map_get(p->second.kills, *weap)))
					db.add_weaps(id, "kills", p->first, *weap, count);
				if((count = map_get(p->second.deaths, *weap)))
					db.add_weaps(id, "deaths", p->first, *weap, count);
			}
			
			if((count = map_get(p->second.flags, FL_CAPTURED)))
				db.add_caps(id, p->first, count);
				
			if(!p->first.is_bot())
			{
				if((count = p->second.logged_time))
					db.add_time(id, p->first, count);

				for(siz_map_citer wu = p->second.weapon_usage.begin(); wu != p->second.weapon_usage.end(); ++wu)
					db.add_weapon_usage(id, p->first, wu->first, wu->second);

				for(moddmg_map_citer md = p->second.mod_damage.begin(); md != p->second.mod_damage.end(); ++md)
					db.add_mod_damage(id, p->first, md->first, md->second.hits, md->second.damage, md->second.hitsRecv, md->second.damageRecv);

				db.add_playerstats(id, p->first,
					p->second.fragsFace, p->second.fragsBack, p->second.fraggedInFace, p->second.fraggedInBack,
					p->second.spawnKills, p->second.spawnKillsRecv, p->second.pushes, p->second.pushesRecv,
					p->second.healthPickedUp, p->second.armorPickedUp);
			}		
		}

		for(onevone_citer o = onevone.begin(); o != onevone.end(); ++o)
			for(guid_siz_citer p = o->second.begin(); p != o->second.end(); ++p)
				db.add_ovo(id, o->first, p->first, p->second);
	}

	for(guid_str_map::iterator player = katina.players.begin(); player != katina.players.end(); ++player)
		if(!player->first.is_bot())
			db.add_player(player->first, player->second);

	db.off();

	return true;
}

bool KatinaPluginStats::shutdown_game(siz min, siz sec)
{
	// bug("in_game: " << in_game);
	in_game = false;
	if(!active)
		return true;
	return true;
}

bool KatinaPluginStats::warmup(siz min, siz sec)
{
	// bug("in_game: " << in_game);
	in_game = false;
	if(!active)
		return true;
	return true;
}

bool KatinaPluginStats::client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name)
{
	// bug("in_game: " << in_game);
	if(!in_game)
		return true;
	if(!active)
		return true;

	std::time_t now = std::time(0);

	if(stats[katina.clients[num]].joined_time)
		stats[katina.clients[num]].logged_time += now - stats[katina.clients[num]].joined_time;

	if(katina.teams[katina.clients[num]] == TEAM_R || katina.teams[katina.clients[num]] == TEAM_B)
		stats[katina.clients[num]].joined_time = now;
	else
		stats[katina.clients[num]].joined_time = 0;

	return true;
}
bool KatinaPluginStats::client_connect(siz min, siz sec, siz num)
{
	if(!active)
		return true;
	// bug("in_game: " << in_game);
}
bool KatinaPluginStats::client_disconnect(siz min, siz sec, siz num)
{
	// bug("in_game: " << in_game);
	if(!in_game)
		return true;
	if(!active)
		return true;

	std::time_t now = std::time(0);

	if(stats[katina.clients[num]].joined_time)
		stats[katina.clients[num]].logged_time += now - stats[katina.clients[num]].joined_time;
	stats[katina.clients[num]].joined_time = 0;

	return true;
}
bool KatinaPluginStats::kill(siz min, siz sec, siz num1, siz num2, siz weap)
{
	// bug("in_game: " << in_game);
	if(!in_game)
		return true;
	if(!active)
		return true;

	if(katina.clients.find(num1) != katina.clients.end() && katina.clients.find(num2) != katina.clients.end())
	{
		if(num1 == 1022 && !katina.clients[num2].is_bot()) // no killer
			++stats[katina.clients[num2]].deaths[weap];
		else if(!katina.clients[num1].is_bot() && !katina.clients[num2].is_bot())
		{
			if(num1 != num2)
			{
				++stats[katina.clients[num1]].kills[weap];
				++onevone[katina.clients[num1]][katina.clients[num2]];
			}
			++stats[katina.clients[num2]].deaths[weap];
		}
	}

	return true;
}
bool KatinaPluginStats::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	// bug("in_game: " << in_game);
	if(!in_game)
		return true;
	if(!active)
		return true;

	if(!katina.clients[num].is_bot())
		++stats[katina.clients[num]].flags[act];

	return true;
}
bool KatinaPluginStats::award(siz min, siz sec, siz num, siz awd)
{
	// bug("in_game: " << in_game);
	if(!in_game)
		return true;
	if(!active)
		return true;

	++stats[katina.clients[num]].awards[awd];

	return true;
}

bool KatinaPluginStats::init_game(siz min, siz sec, const str_map& cvars)
{
	// bug("in_game: " << in_game);
	if(in_game)
		return true;
	in_game = true;

	stats.clear();
	onevone.clear();

	if(!active)
		return true;

	return true;
}


bool KatinaPluginStats::weapon_usage(siz min, siz sec, siz num, siz weapon, siz shots)
{
	bug("KatinaPluginStats::weapon_usage");

	if(!in_game)
		return true;
	if(!active)
		return true;

	if(!katina.clients[num].is_bot())
		stats[katina.clients[num]].weapon_usage[weapon] += shots;
		
	return true;
}

bool KatinaPluginStats::mod_damage(siz min, siz sec, siz num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv)
{
	bug("KatinaPluginStats::mod_damage");

	if(!in_game)
		return true;
	if(!active)
		return true;

	if(!katina.clients[num].is_bot())
	{
		mod_damage_stats& moddmg = stats[katina.clients[num]].mod_damage[mod];
		moddmg.hits       += hits;
		moddmg.damage     += damage;
		moddmg.hitsRecv   += hitsRecv;
		moddmg.damageRecv += damageRecv;
	}
	
	return true;
}

bool KatinaPluginStats::player_stats(siz min, siz sec, siz num,
	siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
	siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
	siz healthPickedUp, siz armorPickedUp)
{
	bug("KatinaPluginStats::player_stats");

	if(!in_game)
		return true;
	if(!active)
		return true;

	if(!katina.clients[num].is_bot())
	{
		struct stats& s = stats[katina.clients[num]];
		s.fragsFace      += fragsFace;
		s.fragsBack      += fragsBack;
		s.fraggedInFace  += fraggedInFace;
		s.fraggedInBack  += fraggedInBack;
		s.spawnKills     += spawnKills;
		s.spawnKillsRecv += spawnKillsRecv;
		s.pushes         += pushes;
		s.pushesRecv     += pushesRecv;
		s.healthPickedUp += healthPickedUp;
		s.armorPickedUp  += armorPickedUp;
	}
	
	return true;
}


void KatinaPluginStats::close()
{

}

}} // katina::plugin
