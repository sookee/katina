
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
, mapname(katina.mapname)
, clients(katina.clients)
, players(katina.players)
, teams(katina.teams)
, server(katina.server)
, active(true)
, write(true)
, in_game(false)
, have_bots(false)
, human_players_r(0)
, human_players_b(0)
, carrierBlue(-1)
, carrierRed(-1)
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
	
	katina.add_var_event(this, "stats.active", active, false);
	katina.add_var_event(this, "stats.write", write, false);
	katina.add_var_event<siz_set>(this, "stats.weaps", db_weaps);
	
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
	katina.add_log_event(this, SAY);
	
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
	//bug("in_game: " << in_game);
	if(!in_game)
		return true;
	in_game = false;
	if(!active)
		return true;
		
	// in game timing
	std::time_t logged_time = 0;
	for(guid_stat_iter p = stats.begin(); p != stats.end(); ++p)
	{
		if(p->second.joined_time)
		{
			p->second.logged_time += katina.now - p->second.joined_time;
			p->second.joined_time = 0;
		}
		logged_time += p->second.logged_time;
	}
	
	if(logged_time)
	{
		if(write)
			db.on();
	
		game_id id = db.add_game(host, port, mapname);
	
		if(id != null_id && id != bad_id)
		{
			for(guid_stat_citer p = stats.begin(); p != stats.end(); ++p)
			{
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
					db.add_player(p->first, p->second.name);
					
					if((count = p->second.logged_time))
						db.add_time(id, p->first, count);
	
					for(siz_map_citer wu = p->second.weapon_usage.begin(); wu != p->second.weapon_usage.end(); ++wu)
						db.add_weapon_usage(id, p->first, wu->first, wu->second);
	
					for(moddmg_map_citer md = p->second.mod_damage.begin(); md != p->second.mod_damage.end(); ++md)
						db.add_mod_damage(id, p->first, md->first, md->second.hits, md->second.damage, md->second.hitsRecv, md->second.damageRecv);
	
					if(p->second.fragsFace | p->second.fragsBack | p->second.fraggedInFace | p->second.fraggedInBack |
						p->second.spawnKills | p->second.spawnKillsRecv | p->second.pushes | p->second.pushesRecv |
						p->second.healthPickedUp | p->second.armorPickedUp | p->second.holyShitFrags | p->second.holyShitFragged |
						p->second.carrierFrags | p->second.carrierFragsRecv)
					{
						db.add_playerstats(id, p->first,
							p->second.fragsFace, p->second.fragsBack, p->second.fraggedInFace, p->second.fraggedInBack,
							p->second.spawnKills, p->second.spawnKillsRecv, p->second.pushes, p->second.pushesRecv,
							p->second.healthPickedUp, p->second.armorPickedUp, p->second.holyShitFrags, p->second.holyShitFragged,
							p->second.carrierFrags, p->second.carrierFragsRecv);
					}
				}		
			}
	
			for(onevone_citer o = onevone.begin(); o != onevone.end(); ++o)
				for(guid_siz_map_citer p = o->second.begin(); p != o->second.end(); ++p)
					db.add_ovo(id, o->first, p->first, p->second);
		}
	
//		for(guid_str_map_citer player = players.begin(); player != players.end(); ++player)
//			if(!player->first.is_bot())
//				db.add_player(player->first, player->second);
//		for(guid_str_map_citer player = names.begin(); player != names.end(); ++player)
//			if(!player->first.is_bot())
//				db.add_player(player->first, player->second);
	
		db.off();
	}
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

void KatinaPluginStats::stall_client(siz num)
{
	if(!stats[clients[num]].joined_time)
		return;
	
	// if we have been recording time for this player, accumulate it
	stats[clients[num]].logged_time += katina.now - stats[clients[num]].joined_time;
	
	plog("\t\t" << players[clients[num]] << ": LT: " << stats[clients[num]].logged_time);
	
	// stop recording time for this player
	stats[clients[num]].joined_time = 0;
}

void KatinaPluginStats::unstall_client(siz num)
{
	if(stats[clients[num]].joined_time)
		return;
	if(teams[clients[num]] != TEAM_R && teams[clients[num]] != TEAM_B)
		return;
	stats[clients[num]].joined_time = katina.now;
	plog("\t\t" << players[clients[num]] << ": JT: " << stats[clients[num]].joined_time);
}

void KatinaPluginStats::stall_clients()
{
	plog("  STALL CLIENTS: " << katina.now);
	for(siz_guid_map_citer ci = clients.begin(); ci != clients.end(); ++ci)
		stall_client(ci->first);
}

void KatinaPluginStats::unstall_clients()
{
	plog("UNSTALL CLIENTS: " << katina.now);
	for(siz_guid_map_citer ci = clients.begin(); ci != clients.end(); ++ci)
		unstall_client(ci->first);
}

void KatinaPluginStats::check_bots_and_players(std::time_t now, siz num)
{
	bool had_bots = have_bots;
	bool human_players_nr_or_nb = !human_players_r || !human_players_b;

	have_bots = false;
	human_players_r = 0;
	human_players_b = 0;
	
	for(guid_siz_map_citer ci = teams.begin(); ci != teams.end(); ++ci)
	{
		if(num != siz(-1) && ci->first == num)
			continue;
		if(ci->first.is_bot())
			have_bots = true;
		else if(ci->second == TEAM_R)
			++human_players_r;
		else if(ci->second == TEAM_B)
			++human_players_b;
	}
	
	bug_var(have_bots);
	bug_var(human_players_r);
	bug_var(human_players_b);

	if(have_bots || !human_players_r || !human_players_b)
		stall_clients();
	else
		unstall_clients();
}

bool KatinaPluginStats::client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name)
{
	bug("KatinaPluginStats::client_userinfo_changed: [" <<  guid << "] " << name << " now: " << katina.now);
	bug("in_game: " << in_game);
	std::cout << std::endl;
	
	if(!guid.is_bot())
		stats[guid].name = name;
		//names[guid] = name;
	
	if(!in_game)
		return true;
	if(!active)
		return true;

	stall_client(num);
	
	check_bots_and_players(katina.now);
	
	if(have_bots)
		return true;
	
	if(!human_players_r || !human_players_b)
		return true;
	
	unstall_client(num);
		
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
	bug("KatinaPluginStats::client_disconnect: [" <<  clients[num] << "] " << players[clients[num]] << " now: " << katina.now);
	bug("in_game: " << in_game);
	std::cout << std::endl;
	if(!in_game)
		return true;
	if(!active)
		return true;

	stall_client(num);
	//if(stats[clients[num]].joined_time)
	//	stats[clients[num]].logged_time += katina.now - stats[clients[num]].joined_time;
	//stats[clients[num]].joined_time = 0;

	check_bots_and_players(katina.now, num);

	return true;
}
bool KatinaPluginStats::kill(siz min, siz sec, siz num1, siz num2, siz weap)
{
	// bug("in_game: " << in_game);
	if(!in_game)
		return true;
	if(!active)
		return true;
	if(have_bots)
		return true;
	if(!human_players_r || !human_players_b)
		return true;

	if(clients.find(num1) != clients.end() && clients.find(num2) != clients.end())
	{
		// Target is human
		if(!clients[num2].is_bot())
		{
			if(num1 == 1022) // no killer
				++stats[clients[num2]].deaths[weap];
			else if(!clients[num1].is_bot())
			{
				if(num1 != num2)
				{
					++stats[clients[num1]].kills[weap];
					++onevone[clients[num1]][clients[num2]];
					
					// Target was a flag carrier
					if(num2 == carrierRed || num2 == carrierBlue)
					{
						++stats[clients[num1]].carrierFrags;
						++stats[clients[num2]].carrierFragsRecv;
					}
				}
				++stats[clients[num2]].deaths[weap];
			}
		}
	}

	return true;
}
bool KatinaPluginStats::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	// bug("in_game: " << in_game);
	if(!in_game)
		return true;
		
	// Remember who is carrying the flag
	if(team == TEAM_R)
		carrierRed = act == 0 ? num : -1;
	else if(team == TEAM_B)
		carrierBlue = act == 0 ? num : -1;
		
	if(!active)
		return true;
	if(have_bots)
		return true;
	if(!human_players_r || !human_players_b)
		return true;

	if(!clients[num].is_bot())
		++stats[clients[num]].flags[act];

	return true;
}
bool KatinaPluginStats::award(siz min, siz sec, siz num, siz awd)
{
	// bug("in_game: " << in_game);
	if(!in_game)
		return true;
	if(!active)
		return true;
	if(have_bots)
		return true;
	if(!human_players_r || !human_players_b)
		return true;

	++stats[clients[num]].awards[awd];

	return true;
}

bool KatinaPluginStats::init_game(siz min, siz sec, const str_map& cvars)
{
	stats.clear();
	onevone.clear();
	//names.clear();

	if(in_game)
		return true;
	in_game = true;


	if(!active)
		return true;

	return true;
}


bool KatinaPluginStats::weapon_usage(siz min, siz sec, siz num, siz weapon, siz shots)
{
	if(!in_game)
		return true;
	if(!active)
		return true;
	if(have_bots)
		return true;
	if(!human_players_r || !human_players_b)
		return true;

	if(!clients[num].is_bot())
		stats[clients[num]].weapon_usage[weapon] += shots;
		
	return true;
}

bool KatinaPluginStats::mod_damage(siz min, siz sec, siz num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv)
{
	if(!in_game)
		return true;
	if(!active)
		return true;
	if(have_bots)
		return true;
	if(!human_players_r || !human_players_b)
		return true;

	if(!clients[num].is_bot())
	{
		mod_damage_stats& moddmg = stats[clients[num]].mod_damage[mod];
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
	siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged)
{
	if(!in_game)
		return true;
	if(!active)
		return true;
	if(have_bots)
		return true;
	if(!human_players_r || !human_players_b)
		return true;

	if(!clients[num].is_bot())
	{
		struct stats& s = stats[clients[num]];
		s.fragsFace        += fragsFace;
		s.fragsBack        += fragsBack;
		s.fraggedInFace    += fraggedInFace;
		s.fraggedInBack    += fraggedInBack;
		s.spawnKills       += spawnKills;
		s.spawnKillsRecv   += spawnKillsRecv;
		s.pushes           += pushes;
		s.pushesRecv       += pushesRecv;
		s.healthPickedUp   += healthPickedUp;
		s.armorPickedUp    += armorPickedUp;
		s.holyShitFrags    += holyShitFrags;
		s.holyShitFragged  += holyShitFragged;
	}
	
	return true;
}

bool KatinaPluginStats::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;
	
	str cmd;
	siss iss(text);
	
	if(!(iss >> cmd))
		return true;
	
	if(cmd == "!register")
	{
		if(write && players[guid] != "UnnamedPlayer" && players[guid] != "RenamedPlayer")
		{
			db.on();
			if(db.set_preferred_name(guid, players[guid]))
				server.chat("^7" + players[guid] + "^7: ^3Your preferred name has been registered.");
			db.off();
		}
	}
	else if(cmd == "!stats" || cmd == "?stats")
	{
		if(cmd[0] == '?')
		{
			server.chat("^7STATS: ^2!stats^7: ^3display a players ^7fph (^2frags^7/^2hour^7) ^2& ^7cph (^2caps^7/^2hour^7)");
			server.chat("^7STATS: ^2!stats^7: ^3calculated for this map and since the start of this month.");
			return true;
		}
		
		siz prev = 0; // count $prev month's back
		if(!(iss >> prev))
			prev = 0;
		
		bug_var(prev);

		bug("getting stats");
		db.on();
		str stats;
		if(db.get_ingame_stats(guid, mapname, prev, stats))
			server.chat("^7STATS: " + players[guid] + "^7: " + stats);
		db.off();
	}
/*	else if(cmd == "!champ") // last month's champion
	{				
		bug("champ:");

		if(ka_cfg.do_db)
		{
			bug("getting champ");
			db.on();
			str stats;
			GUID guid;
			//if(db.get_ingame_champ(mapname, guid, stats))
			//	server.chat("^7LAST MONTH'S CHAMPION: " + players[guid] + "^7: " + stats);
			db.off();
		}
	}
*/	else if(cmd == "!boss" || cmd == "?boss") // best player in this game (from current months stats)
	{				
		if(cmd[0] == '?')
		{
			server.chat("^7STATS: ^2!boss^7: ^3display this map's best player and their ^2!stats ^3for this month.");
			return true;
		}
		
		bug("getting boss");
		db.on();
		str stats;
		GUID guid;
		if(db.get_ingame_boss(mapname, clients, guid, stats) && guid != null_guid)
			server.chat("^7BOSS: " + players[guid] + "^7: " + stats);
		else
			server.chat("^7BOSS: ^3There is no boss on this map");
		db.off();
	}
	return true;
}

void KatinaPluginStats::close()
{

}

}} // katina::plugin
