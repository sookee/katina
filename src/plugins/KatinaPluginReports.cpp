
#include "KatinaPluginReports.h"

#include <katina/KatinaPlugin.h>
#include "KatinaPluginStats.h"

#include <katina/Database.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>
#include <katina/utils.h>

#include <dlfcn.h>
#include <gcrypt.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;
using namespace oastats::utils;

KATINA_PLUGIN_TYPE(KatinaPluginReports);
KATINA_PLUGIN_INFO("katina::reports", "Katina Reports", "0.1");

const str HUD_FLAG_P = "⚑";
const str HUD_FLAG_DIE = "*";
const str HUD_FLAG_CAP = "Y";
const str HUD_FLAG_NONE = ".";
const str HUD_FLAG_RETURN = "^";

const siz TEAM_U = 0;
const siz TEAM_R = 1;
const siz TEAM_B = 2;
const siz TEAM_S = 3;

str hud_flag[2] = {HUD_FLAG_NONE, HUD_FLAG_NONE};
const str flag[2] = {"^1RED", "^4BLUE"};

str weapons[] =
{
	"unknown weapon"
	, "shotgun"
	, "gauntlet"
	, "machinegun"
	, "grenade"
	, "grenade schrapnel"
	, "rocket"
	, "rocket blast"
	, "plasma"
	, "plasma splash"
	, "railgun"
	, "lightening"
	, "BFG"
	, "BFG fallout"
	, "dround"
	, "slimed"
	, "burnt up in lava"
	, "crushed"
	, "telefraged"
	, "falling to far"
	, "suicide"
	, "target lazer"
	, "inflicted pain"
	, "nailgun"
	, "chaingun"
	, "proximity mine"
	, "kamikazi"
	, "juiced"
	, "grappled"
};

siz map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.end() ? 0 : m.at(key);
}

str get_hud(siz m, siz s, str hud_flag[2])
{
	soss oss;
	oss << "00[15" << (m < 10?"0":"") << m << "00:15" << (s < 10?"0":"") << s << " ";
	oss << "04" << hud_flag[FL_RED];
	oss << "02" << hud_flag[FL_BLUE];
	oss << "00]";
	return oss.str();
}

/**
 *
 * @param var
 * @param w
 * @param j - junk (control codes not included in final width)
 */
void set_width(str& var, siz w, siz j)
{
	w += j;
	if(var.size() < w)
		var = str(w - var.size(), ' ') + var;
}

RemoteClientList::RemoteClientList(Katina& katina)
{
}

RemoteClientList::~RemoteClientList()
{
	for(siz i = 0; i < clients.size(); ++i)
		delete clients[i];
}

bool RemoteClientList::send(const str& cmd, str& res)
{
	bug("RemoteClientList::send(" << cmd << ", " << res << ")");
	str ress, sep;
	for(siz i = 0; i < clients.size(); ++i)
	{
		clients[i]->send(cmd, ress);
		res += sep + ress;
		sep = "\n";
	}
	return true;
}

KatinaPluginReports::KatinaPluginReports(Katina& katina)
: KatinaPlugin(katina)
, stats(0)
, client(katina)
, active(false)
, do_flags(false)
, do_flags_hud(false)
, do_chats(false)
, do_kills(false)
, do_infos(false)
, do_stats(false)
//, stats_cols(0)
, spamkill(false)
, spam_limit(2)
{
}

bool KatinaPluginReports::open()
{
	if(katina.get_plugin("katina::stats", "0.0", stats))
		plog("Found: " << stats->get_name());
		
	if(katina.get_plugin("katina::votes", "0.0", votes))
		plog("Found: " << votes->get_name());

	client.off();
	client.clear();
	
	str_vec clients = katina.get_vec("remote.irc.client");
	
	for(siz i = 0; i < clients.size(); ++i)
	{
		log("Creating client: " << clients[i]);
		RemoteClient* c = RemoteClient::create(katina, clients[i]);
		if(c)
		{
			c->on();
			client.add(c);
		}
	}
	
	client.on();
	
	client.chat('*', "^3Stats Reporting System v^7" + katina.get_version() + " - ^1ONLINE");
	
	notspam = katina.get_vec("reports.notspam");

	katina.add_var_event(this, "reports.active", active, false);
	katina.add_var_event(this, "reports.flags", do_flags, false);
	katina.add_var_event(this, "reports.flags.hud", do_flags_hud, false);
	katina.add_var_event(this, "reports.chats", do_chats, false);
	katina.add_var_event(this, "reports.kills", do_kills, false);
	katina.add_var_event(this, "reports.infos", do_infos, false);
	katina.add_var_event(this, "reports.stats", do_stats, false);
	katina.add_var_event(this, "reports.stats.cols", stats_cols);
	katina.add_var_event(this, "reports.stats.sort", stats_sort);
	katina.add_var_event(this, "reports.spam.kill", spamkill, false);
	katina.add_var_event(this, "reports.spam.limit", spam_limit, (siz) 2); 

	katina.add_log_event(this, EXIT);
	katina.add_log_event(this, KILL);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, SAY); 

	return true;
}

str KatinaPluginReports::get_id() const
{
	return ID;
}

str KatinaPluginReports::get_name() const
{
	return NAME;
}

str KatinaPluginReports::get_version() const
{
	return VERSION;
}

bool KatinaPluginReports::init_game(siz min, siz sec, const str_map& cvars)
{
	flags[FL_RED] = 0;
	flags[FL_BLUE] = 0;
	caps.clear();

	if(do_infos && katina.mapname != old_mapname)
	{
		str vote;
		
		if(votes)
		{
			siz love, hate;
			votes->get_votes(love, hate);
			soss oss;
			oss << " ^7" << love << " ^1LOVE ^7" << hate << " ^2HATE ^3==";
			vote = oss.str();
		}
		
		//client.chat('i', ".");
//		client.chat('i', "^3=== Playing Map: ^7" + katina.mapname + "^3 ==" + vote);
		client.chat('i', "^3===" + vote + " ^4map: ^7" + katina.mapname);

		old_mapname = katina.mapname;
	}

	return true;
}

bool KatinaPluginReports::kill(siz min, siz sec, siz num1, siz num2, siz weap)
{
	if(!do_kills)
		return true;
	
	str hud;
	str nums_team = get_nums_team(num1);
	
	if(do_flags && do_flags_hud)
	{
		hud = get_hud(min, sec, hud_flag);
	}
	
	if(weap != MOD_SUICIDE && katina.clients.find(num1) != katina.clients.end() && katina.clients.find(num2) != katina.clients.end())
		client.raw_chat('k', hud + oa_to_IRC(nums_team + "^7" + katina.players[katina.clients[num1]] + " ^4killed ^7" + katina.players[katina.clients[num2]]
			+ " ^4with a ^7" + weapons[weap]));

	return true;
}

str KatinaPluginReports::get_nums_team(siz num)
{
	return get_nums_team(katina.clients[num]);
}

str KatinaPluginReports::get_nums_team(const GUID& guid)
{
	if(katina.teams[guid] == TEAM_R)
		return "^7[^1R^7]";
	else if(katina.teams[guid] == TEAM_B)
		return "^7[^4B^7]";
	else if(katina.teams[guid] == TEAM_S)
		return "^7[^3S^7]";
	return "^7[^2U^7]";
}

bool KatinaPluginReports::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	siz pcol = team - 1; // make 0-1 for array index
	siz ncol = pcol ? 0 : 1;

	str nums_team = get_nums_team(num);

	str hud;

	if(act == FL_CAPTURED)
	{
		++flags[team - 1];
		++caps[katina.clients[num]];
		siz c = caps[katina.clients[num]];
		str msg = katina.players[katina.clients[num]]
			+ "^3 has ^7" + to_string(c) + "^3 flag" + (c==1?"":"s") + "!";
		
		katina.server.cp(msg);
		
		if(do_flags && do_flags_hud)
		{
			hud_flag[pcol] = HUD_FLAG_CAP;
			hud = get_hud(min, sec, hud_flag);
		}
		if(do_flags)
			client.raw_chat('f', hud + oa_to_IRC(nums_team + " " + msg));

		if(do_flags && do_flags_hud)
		{
			hud_flag[pcol] = HUD_FLAG_NONE;
			hud = get_hud(min, sec, hud_flag);
		}
		if(do_flags)
			client.raw_chat('f', hud + oa_to_IRC("^7[ ] ^1RED^3: ^7" + to_string(flags[FL_BLUE]) + " ^3v ^4BLUE^3: ^7" + to_string(flags[FL_RED])));
	}
	else if(act == FL_TAKEN)
	{
		if(do_flags && do_flags_hud)
		{
			hud_flag[pcol] = HUD_FLAG_P;
			hud = get_hud(min, sec, hud_flag);
		}
		if(do_flags)
			client.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + katina.players[katina.clients[num]] + "^3 has taken the " + flag[pcol] + " ^3flag!"));
	}
	else if(act == FL_DROPPED)
	{
		if(do_flags && do_flags_hud)
		{
			hud_flag[ncol] = HUD_FLAG_DIE;
			hud = get_hud(min, sec, hud_flag);
			hud_flag[ncol] = HUD_FLAG_NONE;
		}
		if(do_flags)
			client.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + katina.players[katina.clients[num]] + "^3 has killed the " + flag[ncol] + " ^3flag carrier!"));
	}
	else if(act == FL_RETURNED)
	{
		if(do_flags && do_flags_hud)
		{
			hud_flag[pcol] = HUD_FLAG_RETURN;
			hud = get_hud(min, sec, hud_flag);
			hud_flag[pcol] = HUD_FLAG_NONE;
		}
		if(do_flags)
			client.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + katina.players[katina.clients[num]] + "^3 has returned the " + flag[pcol] + " ^3flag!"));
	}

	return true;
}

bool KatinaPluginReports::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(do_chats)
	{
		str hud;
		str nums_team = get_nums_team(guid);
		
		if(do_flags && do_flags_hud)
		{
			hud = get_hud(min, sec, hud_flag);
		}
	
		if(!spamkill || ++spam[text] < spam_limit || std::find(notspam.begin(), notspam.end(), text) != notspam.end())
			client.raw_chat('c', hud + oa_to_IRC(nums_team + " ^7" + katina.players[guid] + "^7: ^2" + text));
	}

	return true;
}

str get_acc(const stats& stats, siz weapon = siz(-1))//, siz mod)
{
	static siz_map weap_to_mod;
	if(weap_to_mod.empty())
	{
		weap_to_mod[WP_GAUNTLET] = MOD_GAUNTLET;
		weap_to_mod[WP_MACHINEGUN] = MOD_MACHINEGUN;
		weap_to_mod[WP_SHOTGUN] = MOD_SHOTGUN;
		weap_to_mod[WP_GRENADE_LAUNCHER] = MOD_GRENADE;
		weap_to_mod[WP_ROCKET_LAUNCHER] = MOD_ROCKET;
		weap_to_mod[WP_LIGHTNING] = MOD_LIGHTNING;
		weap_to_mod[WP_RAILGUN] = MOD_RAILGUN;
		weap_to_mod[WP_PLASMAGUN] = MOD_PLASMA;
		weap_to_mod[WP_BFG] = MOD_BFG;
		weap_to_mod[WP_GRAPPLING_HOOK] = MOD_GRAPPLE;
		weap_to_mod[WP_NAILGUN] = MOD_NAIL;
		weap_to_mod[WP_PROX_LAUNCHER] = MOD_PROXIMITY_MINE;
		weap_to_mod[WP_CHAINGUN] = MOD_CHAINGUN;
	}
	
	siz ws = WP_GAUNTLET;
	siz we = WP_CHAINGUN;

	if(weapon != siz(-1))
		ws = we = weapon;
	
	siz shots = 0;
	siz hits  = 0;
	
	for(siz w = ws; w <= we; ++w)
	{
		shots += map_get(stats.weapon_usage, w);
		moddmg_map_citer it = stats.mod_damage.find(weap_to_mod[w]);
		if(it != stats.mod_damage.end())
			hits += it->second.hits;
	}
		
	// Pushes also count as hits
	hits += stats.pushes;
		
	str acc = "";
	if(shots > 0)
	{
		double a = ((double) hits / shots) * 100.0;
		acc = to_string(a, 2);
	}
	return acc;
}

siz weapon_to_siz(const str& weapon)
{
	// GA|MG|SG|GL|RL|LG|RG|PG|BG|GH|NG|PL|CG
	static str_siz_map m;
	if(m.empty())
	{
		m[""] = WP_NONE;
		m["GA"] = WP_GAUNTLET;
		m["MG"] = WP_MACHINEGUN;
		m["SG"] = WP_SHOTGUN;
		m["GL"] = WP_GRENADE_LAUNCHER;
		m["RL"] = WP_ROCKET_LAUNCHER;
		m["LG"] = WP_LIGHTNING;
		m["RG"] = WP_RAILGUN;
		m["PG"] = WP_PLASMAGUN;
		m["BG"] = WP_BFG;
		m["GH"] = WP_GRAPPLING_HOOK;
		m["NG"] = WP_NAILGUN;
		m["PL"] = WP_PROX_LAUNCHER;
		m["CG"] = WP_CHAINGUN;
	}
	return m[weapon];
}

bool KatinaPluginReports::exit(siz min, siz sec)
{
	// erase non spam marked messages
	for(str_siz_map_iter i = spam.begin(); i != spam.end();)
	{
		if(!(i->second < spam_limit))
			++i;
		else
		{
			spam.erase(i->first);
			i = spam.begin();
		}
	}
	
//	if(stats)
	{
		typedef std::multimap<siz, GUID> siz_guid_mmap;
		typedef siz_guid_mmap::reverse_iterator siz_guid_mmap_ritr;
		
		siz_guid_mmap sorted;
		
		for(guid_siz_map_citer p = caps.begin(); p != caps.end(); ++p)
			if(p->second)
				sorted.insert(siz_guid_map_pair(p->second, p->first));
		
		if(!sorted.empty())
		{
			siz i = 0;
			siz d = 1;
			siz max = 0;
			siz f = 0; // flags
			str_vec results;
			std::ostringstream oss;
			for(siz_guid_mmap_ritr ri = sorted.rbegin(); ri != sorted.rend(); ++ri)
			{
				++i;
				if(f != ri->first)
					{ d = i; f = ri->first; }
				oss.str("");
				oss << "^3#" << d << " ^7" << katina.players[ri->second] << " ^3capped ^7" << ri->first << "^3 flags.";
				// oss << "^3#" << d << " ^7" << katina.players.at(ri->second) << " ^3capped ^7" << ri->first << "^3 flags.";
				results.push_back(oss.str());
				if(oss.str().size() > max)
					max = oss.str().size();
			}
		
			if(max < 23)
				max = 23;
			katina.server.chat("^5== ^6RESULTS ^5" + str(max - 23, '='));
			for(siz i = 0; i < results.size(); ++i)
				katina.server.chat(results[i]);
			katina.server.chat("^5" + str(max - 12, '-'));
	
			if(do_infos)
			{
				client.chat('i', "^5== ^6RESULTS ^5== ^7"
					+ to_string(flags[FL_BLUE]) + " ^1RED ^7"
					+ to_string(flags[FL_RED]) + " ^4BLUE ^5 ==");
				for(siz i = 0; i < results.size(); ++i)
					client.chat('f', results[i]);
				client.chat('i', "^5" + str(max - 12, '-'));
			}
		}
	}
	
	if(do_stats && stats)
	{
		std::multimap<str, str> scores;
	
		soss oss;
		for(guid_stat_citer p = stats->stats.begin(); p != stats->stats.end(); ++p)
		{
			// %time %fph %cph %fpd %cpd %acc(GA|MG|SG|GL|RL|LG|RG|PG|BG|GH|NG|PL|CG) %name
			// GA MG SG GL RL LG RG PG BG GH NG PL CG

			if(!p->second.logged_time)
				continue;
			str sort; // sort column
			str sort_value; 
			str col;
			siss iss(stats_cols);
			str sep;
			oss.clear();
			oss.str("");
			while(iss >> col)
			{
				if(col == "%time")
				{
					siz min = p->second.logged_time / 60;
					siz sec = p->second.logged_time % 60;
					str mins = to_string(min);
					str secs = to_string(sec);
					if(mins.size() < 2)
						mins = str(2 - mins.size(), ' ') + mins;
					if(secs.size() < 2)
						secs = str(2 - secs.size(), '0') + secs;
					str s = "^7" + mins + "^3:^7" + secs;
					set_width(s, 5, 6);
					oss << sep << s;
					sep = "^2|";
					if(col == stats_sort)
						sort_value = s;
				}
				else if(col == "%fph")
				{
					siz f = 0;
					for(siz i = 0; i < MOD_MAXVALUE; ++i)
						f += map_get(p->second.kills, i);
					siz h = p->second.logged_time;
					
					str fph;
					if(h)
					{
						siz fh = (f * 60 * 60) / h;
						fph = to_string(fh, 3);
					}
					
					str s = "^7" + fph;
					set_width(s, 3, 2);
					oss << sep << s;
					sep = "^2|";
					if(col == stats_sort)
						sort_value = s;
				}
				else if(col == "%cph")
				{
					siz c = map_get(p->second.flags, FL_CAPTURED);
					siz h = p->second.logged_time;
					
					str cph;
					if(h)
					{
						siz ch = (c * 60 * 60) / h;
						cph = to_string(ch, 2);
					}
					
					str s = "^7" + cph;
					set_width(s, 3, 2);
					oss << sep << s;
					sep = "^2|";
					if(col == stats_sort)
						sort_value = s;
				}
				else if(col == "%fpd")
				{
					siz f = 0;
					for(siz i = 0; i < MOD_MAXVALUE; ++i)
						f += map_get(p->second.kills, i);
					siz d = 0;
					for(siz i = 0; i < MOD_MAXVALUE; ++i)
						d += map_get(p->second.deaths, i);
					
					str fpd;
					if(d)
					{
						double fd = double(f) / d;
						fpd = to_string(fd, 5);
					}
					
					str s = "^7" + fpd;
					set_width(s, 5, 2);
					oss << sep << s;
					sep = "^2|";
					if(col == stats_sort)
						sort_value = s;
				}
				else if(col == "%cpd")
				{
					siz c = map_get(p->second.flags, FL_CAPTURED);
					siz d = 0;
					for(siz i = 0; i < MOD_MAXVALUE; ++i)
						d += map_get(p->second.deaths, i);
					
					str cpd;
					if(d)
					{
						double cd = double(c * 100) / d;
						cpd = to_string(cd, 6);
					}
					
					str s = "^7" + cpd;
					set_width(s, 6, 2);
					oss << sep << s;
					sep = "^2|";
					if(col == stats_sort)
						sort_value = s;
				}
				else if(!col.find("%acc"))
				{
					siz w = siz(-1);
					str weapon;
					str skip;
					siss iss(col);
					if(sgl(sgl(iss, skip, '('), weapon, ')'))
						w = weapon_to_siz(weapon);
					else
						w = siz(-1); // all weaps
					
					str acc = get_acc(p->second, w);
					str s = "^7" + acc + "%";
					set_width(s, 7, 2);
					oss << sep << s;
					sep = "^2|";
					if(col == stats_sort)
						sort_value = s;
				}
				else if(col == "%name")
				{
					str name = "unknown";
					if(stats->names.find(p->first) != stats->names.end())
						name = stats->names[p->first];
					oss << sep << "^7" << name;
					if(col == stats_sort)
						sort_value = name; // todo strip this of control codes
				}
			}

			scores.insert(std::make_pair(sort_value, oss.str()));
		}
		if(!scores.empty())
		{
			str sep;
			oss.clear();
			oss.str("");
			str col;
			siss iss(stats_cols);
			while(iss >> col)
			{
				// ] time |fph|cph|fpd  |cpd   |rg acc |
				// ] 13:52|298|  8| 1.68|  4.88| 50.74%|*SoS*THOR
				// ] 14:54|253| 16| 1.37|  8.70| 46.32%|Alien Surf Girl
				// ] 14:54|177| 16| 0.79|  7.14| 32.88%|z
				// ]  5:24|122|  0| 0.85|  0.00| 25.42%|soylent
				// ] 14:54|100| 16| 0.46|  7.41| 27.27%|Evil|Kaeskopf*
				// ]  0:52| 69|  0| 0.33|  0.00| 22.22%|DarkQuiller				sep.clear();
				if(col == "%time")
					{ oss << sep << "^3time "; sep = "^2|"; }
				else if(col == "%fph")
					{ oss << sep << "^3fph"; sep = "^2|"; }
				else if(col == "%cph")
					{ oss << sep << "^3cph"; sep = "^2|"; }
				else if(col == "%fpd")
					{ oss << sep << "^3fpd  "; sep = "^2|"; }
				else if(col == "%cpd")
					{ oss << sep << "^3cpd   "; sep = "^2|"; }
				else if(!col.find("%acc"))
				{
					str weapon = "all";
					str skip;
					siss iss(col);
					if(!sgl(sgl(iss, skip, '('), weapon, ')'))
						weapon = "all";
					oss << sep << "^3" << weapon << " acc%"; sep = "^2|";
				}
			}
			oss << sep;
			client.chat('s', oss.str());
		}
		for(std::multimap<str, str>::reverse_iterator r = scores.rbegin(); r != scores.rend(); ++r)
			client.chat('s', r->second);
	}

	return true;
}

void KatinaPluginReports::close()
{

}

}} // katina::plugin