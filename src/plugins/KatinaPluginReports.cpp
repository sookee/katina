
#include "KatinaPluginReports.h"

#include <katina/KatinaPlugin.h>
#include "KatinaPluginStats.h"

#include <katina/Database.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>

#include <dlfcn.h>
#include <gcrypt.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

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
, stats_cols(0)
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
		bug_var(&client);
		bug_var(c);
		if(c)
		{
			c->on();
			client.add(c);
		}
	}
	
	client.on();
	
	client.chat('*', "^3Stats Reporting System v^7" + get_version() + " - ^1ONLINE");

	katina.add_var_event(this, "report.active", active, false);
	katina.add_var_event(this, "report.flags", do_flags, false);
	katina.add_var_event(this, "report.flags.hud", do_flags_hud, false);
	katina.add_var_event(this, "report.chats", do_chats, false);
	katina.add_var_event(this, "report.kills", do_kills, false);
	katina.add_var_event(this, "report.infos", do_infos, false);
	katina.add_var_event(this, "report.stats", do_stats, false);
	katina.add_var_event(this, "report.stats.cols", stats_cols, 0U);
	katina.add_var_event(this, "report.spam.kill", spamkill, false);
	katina.add_var_event(this, "report.spam.limit", spam_limit, 2U); 

	katina.add_log_event(this, EXIT);
	//katina.add_log_event(this, SHUTDOWN_GAME);
	//katina.add_log_event(this, WARMUP);
	//katina.add_log_event(this, CLIENT_USERINFO_CHANGED);
	//katina.add_log_event(this, CLIENT_CONNECT);
	//katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, KILL);
	katina.add_log_event(this, CTF);
	//katina.add_log_event(this, AWARD);
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

bool KatinaPluginReports::exit(siz min, siz sec)
{
	client.chat('*', "^3Game Over");

	// erase non spam marked messages
	for(str_siz_map_iter i = spam.begin(); i != spam.end();)
	{
		if(i->second < spam_limit)
		{
			spam.erase(i->first);
			i = spam.begin();
		}
		else
			++i;
	}
	
	if(do_flags && stats)
	{
		typedef std::multimap<siz, GUID> siz_guid_mmap;
		typedef siz_guid_mmap::reverse_iterator siz_guid_mmap_ritr;
		
		siz_guid_mmap sorted;
		
		for(guid_stat_citer p = stats->stats.begin(); p != stats->stats.end(); ++p)
			sorted.insert(siz_guid_pair(map_get(p->second.flags, FL_CAPTURED), p->first));
		
		//for(guid_siz_citer c = caps.begin(); c != caps.end(); ++c)
		//	sorted.insert(siz_guid_pair(c->second, c->first));
	
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
			oss << "^3#" << d << " ^7" << katina.players.at(ri->second) << " ^3capped ^7" << ri->first << "^3 flags.";
			results.push_back(oss.str());
			if(oss.str().size() > max)
				max = oss.str().size();
		}
	
		katina.server.chat("^5== ^6RESULTS ^5" + str(max - 23, '='));
		for(siz i = 0; i < results.size(); ++i)
			katina.server.chat(results[i]);
		katina.server.chat("^5" + str(max - 12, '-'));
	
		if(do_infos)
		{
			client.chat('i', "^5== ^6RESULTS ^5== ^7"
				+ to_string(flags[FL_BLUE]) + " ^1RED ^7"
				+ to_string(flags[FL_RED]) + " ^4BLUE ^3 ==");
			for(siz i = 0; i < results.size(); ++i)
				client.chat('f', results[i]);
			client.chat('i', "^5" + str(max - 12, '-'));
		}
	}
	
	if(do_stats && stats)
	{
		std::multimap<double, str> scores;
	
		soss oss;
		oss.str("");
		str sep;
		if(stats_cols & RSC_TIME)
			{ oss << sep << "^3time "; sep = "^2|"; }
		if(stats_cols & RSC_FPH)
			{ oss << sep << "^3fph"; sep = "^2|"; }
		if(stats_cols & RSC_TIME)
			{ oss << sep << "^3cph"; sep = "^2|"; }
		if(stats_cols & RSC_TIME)
			{ oss << sep << "^3fpd  "; sep = "^2|"; }
		if(stats_cols & RSC_TIME)
			{ oss << sep << "^3cpd   "; sep = "^2|"; }
		if(stats_cols & RSC_RGACC)
			{ oss << sep << "^3rg acc  "; sep = "^2|"; }
		client.chat('s', oss.str());
	
		for(guid_stat_citer p = stats->stats.begin(); p != stats->stats.end(); ++p)
		{
			const str& player = katina.players.at(p->first);
			con("player: " << player);
			con("\t  caps: " << map_get(p->second.flags, FL_CAPTURED));
			con("\t kills: " << map_get(p->second.kills, MOD_RAILGUN));
			con("\tdeaths: " << map_get(p->second.deaths, MOD_RAILGUN));
			con("\t  defs: " << map_get(p->second.awards, AW_DEFENCE));
			con("\t gaunt: " << map_get(p->second.awards, AW_GAUNTLET));
			con("\t  time: " << p->second.logged_time << 's');
			// TODO: modify this to add AW options as well as insta

			siz c = map_get(p->second.flags, FL_CAPTURED);
	
			siz k = 0;
			for(siz i = 0; i < MOD_MAXVALUE; ++i)
				k += map_get(p->second.kills, i);
	
			siz d = 0;
			for(siz i = 0; i < MOD_MAXVALUE; ++i)
				d += map_get(p->second.deaths, i);
	
			siz h = p->second.logged_time;

			double rkd = 0.0;
			double rcd = 0.0;
			siz rkh = 0;
			siz rch = 0;
			str kd, cd, kh, ch;
			
			if(d == 0 || h == 0)
			{
				if(d == 0)
				{
					if(k)
						kd = "perf";
					if(c)
						cd = "perf";
				}
				if(h == 0)
				{
					if(k)
						kh = "inf";
					if(c)
						ch = "inf";
				}
			}
			else
			{
				rkd = double(k) / d;
				rcd = double(c * 100) / d;
				rkh = k * 60 * 60 / h;
				rch = c * 60 * 60 / h;
	
				kd = to_string(rkd, 5);
				cd = to_string(rcd, 6);
				kh = to_string(rkh, 3);
				ch = to_string(rch, 2);
			}
			if(k || c || d)
			{
				str mins, secs;
				siz m = p->second.logged_time / 60;
				siz s = p->second.logged_time % 60;
				oss.str("");
				oss << m;
				mins = oss.str();
				oss.str("");
				oss << s;
				secs = oss.str();
				if(mins.size() < 2)
					mins = str(2 - mins.size(), ' ') + mins;
				if(secs.size() < 2)
					secs = str(2 - secs.size(), '0') + secs;
	
				oss.str("");
				str sep, col;
				if(stats_cols & RSC_TIME)
				{
					col = "^7" + mins + "^3:^7" + secs;
					set_width(col, 5, 6);
					oss << sep << col;
					sep = "^2|";
				}
				if(stats_cols & RSC_FPH)
				{
					col = "^7" + kh;
					set_width(col, 3, 2);
					oss << sep << col;
					sep = "^2|";
				}
				if(stats_cols & RSC_CPH)
				{
					col = "^7" + ch;
					set_width(col, 3, 2);
					oss << sep << col;
					sep = "^2|";
				}
				if(stats_cols & RSC_KPD)
				{
					col = "^7" + kd;
					set_width(col, 5, 2);
					oss << sep << col;
					sep = "^2|";
				}
				if(stats_cols & RSC_CPD)
				{
					col = "^7" + cd;
					set_width(col, 6, 2);
					oss << sep << col;
					sep = "^2|";
				}
				if(stats_cols & RSC_RGACC)
				{
					// Calculate railgun accuracy
					siz shots = map_get(p->second.weapon_usage, WP_RAILGUN);
					siz hits  = 0;
					moddmg_map_citer it = p->second.mod_damage.find(MOD_RAILGUN);
					if(it != p->second.mod_damage.end())
						hits = it->second.hits;
						
					// Pushes also count as hits
					hits += p->second.pushes;
						
					str acc = "";
					if(shots > 0)
					{
						double a = ((double) hits / shots) * 100.0;
						acc = to_string(a, 2);
					}
				
					col = "^7" + acc + "%";
					set_width(col, 8, 2);
					oss << sep << col;
					sep = "^2|";
				}
	
				oss << sep << "^7" << player;
				scores.insert(std::make_pair(rkh, oss.str()));
			}
		}
		for(std::multimap<double, str>::reverse_iterator r = scores.rbegin(); r != scores.rend(); ++r)
			client.chat('s', r->second);
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
	if(!do_flags)
		return true;
		
	siz pcol = team - 1; // make 0-1 for array index
	siz ncol = pcol ? 0 : 1;

	str nums_team = get_nums_team(num);

	str hud;

	if(act == FL_CAPTURED)
	{
		if(stats)
		{
			siz caps = map_get(stats->stats[katina.clients[num]].flags, FL_CAPTURED);
			str msg = katina.players[katina.clients[num]]
				+ "^3 has ^7" + to_string(caps) + "^3 flag" + (caps==1?"":"s") + "!";
			katina.server.cp(msg);
			if(do_flags_hud)
			{
				hud_flag[pcol] = HUD_FLAG_CAP;
				hud = get_hud(min, sec, hud_flag);
			}
			client.raw_chat('f', hud + oa_to_IRC(nums_team + " " + msg));
		}
		if(do_flags_hud)
		{
			hud_flag[pcol] = HUD_FLAG_NONE;
			hud = get_hud(min, sec, hud_flag);
		}
		client.raw_chat('f', hud + oa_to_IRC("^7[ ] ^1RED^3: ^7" + to_string(flags[FL_BLUE]) + " ^3v ^4BLUE^3: ^7" + to_string(flags[FL_RED])));
	}
	else if(act == FL_TAKEN)
	{
		if(do_flags_hud)
		{
			hud_flag[pcol] = HUD_FLAG_P;
			hud = get_hud(min, sec, hud_flag);
		}
		client.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + katina.players[katina.clients[num]] + "^3 has taken the " + flag[pcol] + " ^3flag!"));
	}
	else if(act == FL_DROPPED)
	{
		if(do_flags_hud)
		{
			hud_flag[ncol] = HUD_FLAG_DIE;
			hud = get_hud(min, sec, hud_flag);
			hud_flag[ncol] = HUD_FLAG_NONE;
		}
		client.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + katina.players[katina.clients[num]] + "^3 has killed the " + flag[ncol] + " ^3flag carrier!"));
	}
	else if(act == FL_RETURNED)
	{
		if(do_flags_hud)
		{
			hud_flag[pcol] = HUD_FLAG_RETURN;
			hud = get_hud(min, sec, hud_flag);
			hud_flag[pcol] = HUD_FLAG_NONE;
		}
		client.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + katina.players[katina.clients[num]] + "^3 has returned the " + flag[pcol] + " ^3flag!"));
	}

	return true;
}

//bool KatinaPluginReports::award(siz min, siz sec, siz num, siz awd)
//{
//	return true;
//}

bool KatinaPluginReports::init_game(siz min, siz sec, const str_map& cvars)
{
	flags[FL_RED] = 0;
	flags[FL_BLUE] = 0;

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
		
		client.chat('i', ".");
		client.chat('i', "^3== Playing Map: ^7" + katina.mapname + "^3 ==" + vote);

		old_mapname = katina.mapname;
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
	
		if(!spamkill || ++spam[text] < spam_limit)
			client.raw_chat('c', hud + oa_to_IRC(nums_team + "^7say: " + katina.players[guid] + " ^2" + text));
	}

	return true;
}

//bool KatinaPluginReports::unknown(siz min, siz sec, const str& cmd, const str& params)
//{
//
//}

void KatinaPluginReports::close()
{

}

}} // katina::plugin