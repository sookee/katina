
#include "KatinaPluginAdmin.h"

#include <list>
#include <algorithm>
#include <functional>

#include <ctime>

#include <katina/types.h>
#include <katina/log.h>
#include <katina/str.h>
#include <katina/codes.h>


namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;
using namespace oastats::string;

KATINA_PLUGIN_TYPE(KatinaPluginAdmin);
KATINA_PLUGIN_INFO("katina::admin", "Katina Admin", "0.1-dev");

KatinaPluginAdmin::KatinaPluginAdmin(Katina& katina)
: KatinaPlugin(katina)
, mapname(katina.mapname)
, clients(katina.clients)
, players(katina.players)
, teams(katina.teams)
, server(katina.server)
, active(true)
, total_kills(0)
, total_caps(0)
, policy(policy_t::FT_EVEN_SCATTER)
, spamkill_warn(3)
, spamkill_mute(5)
, spamkill_mute_period(60)
{
}

bool is_ip(const str& s)
{
	siz dot = std::count(s.begin(), s.end(), '.');
	if(dot > 3)
		return false;

	siz dig = std::count_if(s.begin(), s.end(), std::ptr_fun<int, int>(isdigit));
	if(dig > dot * 3 + 3)
		return false;

	return s.size() == dot + dig;
}

bool is_guid(const str& s)
{
	//assert(min <= 8);
	return s.size() == 8
		&& std::count_if(s.begin(), s.end(), std::ptr_fun<int, int>(isxdigit)) == s.size();
}

bool KatinaPluginAdmin::load_total_bans()
{
	sifs ifs((katina.config_dir + "/total-bans.txt").c_str());

	if(!ifs)
	{
		plog("ERROR: can not open bans file.");
		return false;
	}

	str line;
	while(sgl(ifs, line))
	{
		if(trim(line).empty())
			continue;

		if(is_ip(line))
			total_bans.ips.push_back(line);
		else if(is_guid(line))
			total_bans.guids.push_back(line);
	}

	return true;
}

bool KatinaPluginAdmin::load_sanctions()
{
	sifs ifs((katina.config_dir + "/sanctions.dat").c_str());

	if(!ifs)
	{
		plog("ERROR: can not open sanctions file.");
		return false;
	}

	siz n = 0;
	str line;
	katina::plugin::sanction s;
	while(sgl(ifs, line))
	{
		++n;
		if(trim(line).empty())
			continue;
		siss iss(line);
		if(!(iss >> s))
		{
			plog("ERROR: parsing sanction file line: " << n);
			return false;
		}

		if(s.expires && s.expires < std::time(0))
			continue;

		sanctions.push_back(s);
	}

	return true;
}

bool KatinaPluginAdmin::save_sanctions()
{
	sofs ofs((katina.config_dir + "/sanctions.dat").c_str());

	if(!ofs)
	{
		plog("ERROR: can not open sanctions file.");
		return false;
	}

	for(sanction_lst_iter i = sanctions.begin(); i != sanctions.end(); ++i)
		ofs << *i << '\n';

	return true;
}

enum
{
	S_NONE = 0
	, S_MUTEPP
	, S_FIXNAME
	, S_WARN_ON_SIGHT
	, S_RETEAM
};

//BUG: cap_factor: 1 [../../../src/plugins/KatinaPluginAdmin.cpp] (147)
//BUG: clients.size(): 5 [../../../src/plugins/KatinaPluginAdmin.cpp] (148)
//BUG: i->first: 2 [../../../src/plugins/KatinaPluginAdmin.cpp] (151)
//BUG: i->second: DFADEDA3 [../../../src/plugins/KatinaPluginAdmin.cpp] (152)
//BUG: i->first: 3 [../../../src/plugins/KatinaPluginAdmin.cpp] (151)
//BUG: i->second: 7B5DA741 [../../../src/plugins/KatinaPluginAdmin.cpp] (152)
//BUG: i->first: 8 [../../../src/plugins/KatinaPluginAdmin.cpp] (151)
//BUG: i->second: 6AAAA58B [../../../src/plugins/KatinaPluginAdmin.cpp] (152)
//BUG: i->first: 9 [../../../src/plugins/KatinaPluginAdmin.cpp] (151)
//BUG: i->second: 271D5815 [../../../src/plugins/KatinaPluginAdmin.cpp] (152)
//BUG: i->first: 11 [../../../src/plugins/KatinaPluginAdmin.cpp] (151)
//BUG: i->second: E69CED01 [../../../src/plugins/KatinaPluginAdmin.cpp] (152)
//BUG: ------------------------------------------- [../../../src/plugins/KatinaPluginAdmin.cpp] (169)
//BUG: team: 2 [../../../src/plugins/KatinaPluginAdmin.cpp] (171)

bool KatinaPluginAdmin::fixteams(policy_t policy)
{
	siz_mmap rank; // skill -> slot

	siz skill_r = 0;
	siz skill_b = 0;
	siz r = 0;
	siz b = 0;

	double cap_factor = total_caps ? total_kills / total_caps : 1.0;
	bug_var(cap_factor);
	bug_var(clients.size());
	for(siz_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
	{
//		if(i->second.is_bot())
//			continue;

		bug_var(i->first);
		bug_var(i->second);
		bug_var(teams[i->second]);
		 // 1 = red, 2 = blue, 3 = spec
		if(teams[i->second] != TEAM_R && teams[i->second] != TEAM_B)
			continue;

		siz fph = secs[i->first] ? (kills[i->first] * 60 * 60) / secs[i->first] : 0;
		siz cph = secs[i->first] ? (caps[i->first] * 60 * 60) / secs[i->first] : 0;
		siz skill = sqrt(pow(fph, 2) + pow(cph * cap_factor, 2));

		if(teams[i->second] == 1)
			{ skill_r += skill; ++r; }
		else
			{ skill_b += skill; ++b; }

		pbug("-------------------------------------------");
		pbug("FIXTEAMS:  slot: " << i->first);
		pbug("FIXTEAMS:  name: " << players[clients[i->first]]);
		pbug("FIXTEAMS: kills: " << kills[i->first]);
		pbug("FIXTEAMS:  caps: " << caps[i->first]);
		pbug("FIXTEAMS:  secs: " << secs[i->first]);
		pbug("FIXTEAMS:   fph: " << fph);
		pbug("FIXTEAMS:   cph: " << cph);
		pbug("FIXTEAMS: skill: " << skill);

		rank.insert(siz_mmap_pair(skill, i->first));
	}

	pbug("-------------------------------------------");

	if(policy == policy_t::FT_EVEN_SCATTER)
	{
		siz team = (rand() % 2) + 1;
		bug_var(team);
		for(siz_mmap_criter i = rank.rbegin(); i != rank.rend(); ++i)
		{
			pbug("FIXTEAMS: putting: " << i->second << " [" << i->first << "] "
					<< "on team " << str(team == 1 ? "r" : "b"));
//			if(!server.command("!putteam " + to_string(i->second) + " " + str(team == 1 ? "r" : "b")))
//				server.command("!putteam " + to_string(i->second) + " " + str(team == 1 ? "r" : "b")); // one retry
			team = team == 1 ? 2 : 1;
		}
	}
	else if(policy == policy_t::FT_NEAREST_DIFFERENCE)
	{
//		if(r > b && skill_r >= skill_b) // switch one from r to b
//		{
//			const siz diff = (skill_r - skill_b) / 2;
//			siz_mmap_citer match = rank.begin();
//			if(match != rank.end())
//			{
//				siz match_diff = match->first - diff;
//				if(match->first < diff)
//					match_diff = diff = match->first;
//				for(siz_mmap_citer i = match; i != rank.end(); ++i)
//				{
//					if(teams[client[i->second]] != 1) // avoid blue team
//						continue;
//					if(i->first < diff && (diff - i->first) < match_diff)
//						{ match = i; match_diff = diff - i->first; }
//					else if((i->first - diff) < match_diff)
//						{ match = i; match_diff = i->first - diff; }
//				}
//				if(!server.command("!putteam " + to_string(match->second) + " b"))
//					server.command("!putteam " + to_string(match->second) + " b"); // one retry
//			}
//		}
//		else if(b > r && skill_b >= skill_r) // switch one from b to r
//		{
//
//		}
//		else if(r == b) // swap one playerfrom each team
//		{
//
//		}
	}

	return true;
}

bool KatinaPluginAdmin::mutepp(siz num)
{
	str reply;
	server.command("!mute " + to_string(num), reply);
	// ����      broadcast: print "^3!mute: ^7^1S^2oo^3K^5ee^7 has been muted by ^7console\n"
	if(reply.find("broadcast: print") != str::npos
	&& reply.find("has been muted by") != str::npos)
		return true;
	return false;
}

bool KatinaPluginAdmin::fixname(siz num, const str& name)
{
	str reply;
	server.command("!rename " + to_string(num) + " " + name, reply);
	// ����      broadcast: print "^1S^2oo^3K^5ee^7 renamed to wibble\n"
	if(reply.find("broadcast: print") != str::npos
	&& reply.find("renamed to") != str::npos)
		return true;
	return false;
}

bool KatinaPluginAdmin::warn_on_sight(siz num, const str& reason)
{
	str reply;
	server.command("!warn " + to_string(num) + " " + reason, reply);
	// ����
	// ����      ^/warn: no connected player by that name or slot #
	if(reply.find("warn: no connected player by that name or slot #") != str::npos)
		return false;
	return true;
}

siz char_to_team(char t)
{
	// 1 = red, 2 = blue, 3 = spec
	if(toupper(t) == 'R')
		return 1;
	else if(toupper(t) == 'B')
		return 2;
	return 3;
}

bool KatinaPluginAdmin::reteam(siz num, char team)
{
	if(clients.find(num) == clients.end())
	{
		plog("ERROR: can't find client num: " << num);
		return true;
	}
	if(teams.find(clients[num]) == teams.end())
	{
		plog("ERROR: can't find team guid: " << clients[num]);
		return true;
	}
	if(teams[clients[num]] == char_to_team(team))
		return true;

	str reply;
	server.command("!putteam " + to_string(num) + " " + (char)tolower(team), reply);
	// ����
	// ����      ^/warn: no connected player by that name or slot #
	if(reply.find("!putteam: no connected player by that name or slot #") != str::npos
	|| reply.find("!putteam: unknown team") != str::npos)
		return false;
	return true;
}

bool KatinaPluginAdmin::open()
{
	bug_func();

	pbug("Adding var events");
	katina.add_var_event(this, "admin.active", active);
	//katina.add_var_event(this, "flag", "0");
	pbug("Adding log events");
	katina.add_log_event(this, INIT_GAME);
	//katina.add_log_event(this, WARMUP);
	katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_CONNECT_INFO);
	//katina.add_log_event(this, CLIENT_BEGIN);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);
	katina.add_log_event(this, CLIENT_SWITCH_TEAM);
	katina.add_log_event(this, KILL);
	katina.add_log_event(this, CTF);
	//katina.add_log_event(this, CTF_EXIT);
	//katina.add_log_event(this, SCORE_EXIT);
	//katina.add_log_event(this, AWARD);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, CHAT);
	//katina.add_log_event(this, SHUTDOWN_GAME);
	//katina.add_log_event(this, EXIT);
	//katina.add_log_event(this, UNKNOWN);

	pbug("Loading sanctions");
	load_sanctions();
	load_total_bans();

	siz num;
	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end(); ++s)
	{
		if((num = katina.getClientNr(s->guid)) == siz(-1))
			continue;

		if(players.find(s->guid) == players.end())
			continue;

		// !mute++
		if(s->type == S_MUTEPP)
			if(mutepp(num))
				s->applied = true;
		// !fixname
		if(s->type == S_FIXNAME && !s->params.empty()
		&& s->params[0] == players[s->guid])
			if(fixname(num, s->params[0]))
				s->applied = true;
	}

	pbug("setting config");
	active = katina.get("admin.active", false);

	return true;
}

str KatinaPluginAdmin::get_id() const
{
	return ID;
}

str KatinaPluginAdmin::get_name() const
{
	return NAME;
}

str KatinaPluginAdmin::get_version() const
{
	return VERSION;
}

bool KatinaPluginAdmin::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;

	// !muted become unstuck after every game
	siz num;
	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end(); ++s)
	{
		if((num = katina.getClientNr(s->guid)) == siz(-1))
			continue;

		if(players.find(s->guid) == players.end())
			continue;

		if(s->type == S_MUTEPP)
			if(mutepp(num))
				s->applied = true;
	}

	for(siz_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
	{
		if((teams[i->second] == TEAM_R || teams[i->second] == TEAM_B))
		{
			pbug("STARTING TIMER FOR: " << players[i->second] << " [" << katina.now << "]");
			time[i->first] = katina.now;
		}
	}
	return true;
}

bool KatinaPluginAdmin::warmup(siz min, siz sec)
{
	if(!active)
		return true;

	// stop all timers
	for(siz_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
		time[i->first] = 0;

	return true;
}

bool KatinaPluginAdmin::client_connect(siz min, siz sec, siz num)
{
	if(!active)
		return true;
	return true;
}

bool KatinaPluginAdmin::client_connect_info(siz min, siz sec, siz num, const GUID& guid, const str& ip)
{
	if(!active)
		return true;

	for(str_vec_iter i = total_bans.ips.begin(); i != total_bans.ips.end(); ++i)
		if(!ip.find(*i)) // left substring match
			server.command("!ban " + to_string(num) + "AUTO BAN IP: " + *i);

	for(str_vec_iter i = total_bans.guids.begin(); i != total_bans.guids.end(); ++i)
		if(guid == *i)
			server.command("!ban " + to_string(num) + "AUTO BAN GUID: " + *i);

	return true;
}

bool KatinaPluginAdmin::client_begin(siz min, siz sec, siz num)
{
	if(!active)
		return true;
	return true;
}

bool KatinaPluginAdmin::client_disconnect(siz min, siz sec, siz num)
{
	if(!active)
		return true;

	{ // !fixteams
		kills.erase(num);
		caps.erase(num);
		secs.erase(num);
		time[num] = 0;
	}

	return true;
}

bool KatinaPluginAdmin::client_userinfo_changed(siz min, siz sec, siz num, siz team
		, const GUID& guid, const str& name, siz hc)
{
	if(!active)
		return true;
//	plog("client_userinfo_changed(" << num << ", " << team << ", " << guid << ", " << name << ")");
//	plog("clients[" << num << "]         : " << clients[num]);
//	plog("players[clients[" << num << "]]: " << players[clients[num]]);

	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end();)
	{
		if(s->guid != guid)
			{ ++s; continue; }

		if(s->type == S_FIXNAME)
		{
			if(!s->params.empty() && name != s->params[0])
				if(fixname(num, s->params[0]))
					s->applied = true;
			++s;
		}
		else if(s->type == S_WARN_ON_SIGHT)
		{
			if(warn_on_sight(num, s->reason))
				{ s = sanctions.erase(s); save_sanctions(); }
			else
				++s;
		}
		else if(s->type == S_RETEAM && !s->params.empty() && !s->params[0].empty())
		{
			if(reteam(num, s->params[0][0]))
				s->applied = true;
			++s;
		}
	}

	return true;
}

bool KatinaPluginAdmin::client_switch_team(siz min, siz sec, siz num, siz teamBefore, siz teamNow)
{
	if(!active)
		return true;

	plog("client_switch_team(" << num << ", " << teamBefore << ", " << teamNow << ")");

	if(clients[num].is_bot())
		return true;

	if((teamNow == TEAM_R || teamNow == TEAM_B) && !time[num])
	{
		pbug("STARTING TIMER FOR: " << players[clients[num]] << " [" << katina.now << "]");
		time[num] = katina.now; // start timer if not running
	}
	else if(teamNow == TEAM_S && time[num])
	{
		pbug("STOPPING TIMER FOR: " << players[clients[num]] << " after " << (katina.now - time[num]) << " seconds");
		secs[num] += (katina.now - time[num]);
		time[num] = 0; // stop timer if running
	}
	return true;
}

bool KatinaPluginAdmin::kill(siz min, siz sec, siz num1, siz num2, siz weap)
{
	if(!active)
		return true;

	 // !fixteams
	if(time[num1]) // is the timer running?
		++kills[num1];
	++total_kills;
	return true;
}

bool KatinaPluginAdmin::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	if(!active)
		return true;

	if(act == FL_CAPTURED) // !fixteams
	{
		if(time[num]) // is the timer running?
			++caps[num];
		++total_caps;
	}
//	plog("ctf(" << num << ", " << team << ", " << act << ")");
	return true;
}

bool KatinaPluginAdmin::ctf_exit(siz min, siz sec, siz r, siz b)
{
	if(!active)
		return true;
//	plog("ctf_exit(" << r << ", " << b << ")");
	return true;
}

bool KatinaPluginAdmin::score_exit(siz min, siz sec, int score, siz ping, siz num, const str& name)
{
	if(!active)
		return true;
//	plog("score_exit(" << score << ", " << ping << ", " << num << ", " << name << ")");
	return true;	
}

bool KatinaPluginAdmin::award(siz min, siz sec, siz num, siz awd)
{
	if(!active)
		return true;
//	plog("award(" << num << ", " << awd << ")");
	return true;
}

const str ADMIN_ALIASES_FILE_KEY = "admin.aliases.file";
const str ADMIN_ALIASES_FILE_VAL = "admin.aliases.text";

/**
 * Potentially this function could
 * translate commands to avoid conflicting
 * with commands for other plugins.
 */
str KatinaPluginAdmin::trans(const str& cmd) const
{
	static str_map* aliases = 0;

	return cmd; // TODO: remove this

	if(!aliases)
	{
		aliases = new str_map;

		sifs ifs((katina.config_dir + "/"
				+ katina.get(ADMIN_ALIASES_FILE_KEY, ADMIN_ALIASES_FILE_VAL)).c_str());

		if(!ifs)
			return cmd;

		siz pos;
		str line, key, val;
		while(sgl(ifs, line))
		{
			if((pos = line.find("//")) != str::npos)
				line.erase(pos);
			if(trim(line).empty() || line[0] == '#')
				continue;
			else
			{
				siss iss(line);
				if(sgl(sgl(iss, key, ':') >> std::ws, val))
					(*aliases)[key] = val;
			}
		}
	}

	if(aliases->empty())
		return cmd;

	if(aliases->find(cmd) == aliases->end())
		return cmd;

	return (*aliases)[cmd];
}

bool KatinaPluginAdmin::check_admin(const GUID& guid)
{
	if(!katina.is_admin(guid))
	{
		plog("WARN: Admin attempt by non admin player: [" << guid << "] " << players[guid]);
		return false;
	}
	return true;
}

bool KatinaPluginAdmin::check_slot(siz num)
{
	if(katina.check_slot(num))
		return true;
	plog("WARN: Unknown client number: " << num);
	server.chat_nobeep("^7!ADMIN: ^3Unknown client number: ^7" + to_string(num));
	return false;
}

std::time_t parse_duration(const str& duration, std::time_t dflt)
{
	std::time_t t = dflt;

	// 30 w|d|h|m|s

	siss iss(duration);

	str units = "m";
	if(!(iss >> t >> std::ws >> units))
	{
		plog("ERROR: parsing duration: " << duration);
		return dflt;
	}

	if(units == "s")
		t *= 1;
	else if (units == "m")
		t *= 60;
	else if (units == "h")
		t *= 60 * 60;
	else if (units == "d")
		t *= 60 * 60 * 24;
	else if (units == "w")
		t *= 60 * 60 * 24 * 7;

	return t;
}

void KatinaPluginAdmin::remove_sanctions(const GUID& guid, siz type)
{
	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end();)
		if(s->guid == guid && s->type == type)
			s = sanctions.erase(s);
		else
			++s;
	save_sanctions();
}

void KatinaPluginAdmin::spamkill(siz num)
{
	if(!server.command("!mute " + to_string(num)))
		if(!server.command("!mute " + to_string(num))) // 1 retry
			return;
	server.msg_to(num, "^2Your ^7SPAM ^2has triggered ^7auto-mute ^2for ^7" + to_string(spamkill_mute_period) + " ^2seconds");
	mutes[num] = std::time(0);
}

// tODO: make these commands use katina.parse_slot_guid_name()
bool KatinaPluginAdmin::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;

	plog("say(" << guid << ", " << text << ")");

	// !cmd <parans>

	siz say_num;
	if((say_num = katina.getClientNr(guid)) == siz(-1))
	{
		plog("ERROR: Unable to get slot number from guid: " << guid);
		return true;
	}

	if(!check_slot(say_num))
		return true;

	// spamkill
//	std::time_t now = std::time(0);
//	if(mutes[say_num] && mutes[say_num] + spamkill_mute_period < now)
//	{
//		if(!server.command("!unmute " + to_string(say_num)))
//			server.command("!unmute " + to_string(say_num));
//		mutes.erase(say_num);
//	}
//
//	spam s;
//	s.num = say_num;
//	s.when = now;
//	spam_lst& list = spams[text];
//	list.push_back(s);
//
//	siz count = 0;
//	for(spam_lst_iter i = list.begin(); i != list.end();)
//	{
//		if(i->num != say_num)
//			{ ++i; continue; }
//		if(now - i->when > spamkill_period)
//			{ i = list.erase(i); continue; }
//		if(++count > spamkill_mute)
//			{ list.clear(); spamkill(say_num); break; }
//	}

	// /spamkill

	siss iss(text);

	str cmd, params;

	if(!(iss >> cmd >> std::ws))
	{
		plog("ERROR parsing admin command.");
		return true;
	}

	sgl(iss, params);

	iss.clear();
	iss.str(params);

	if(cmd == trans("!help") || cmd == trans("?help"))
	{
		server.msg_to(say_num, "^7ADMIN: ^2?request^7", true);

		if(!check_admin(guid))
			return true;

		server.msg_to(say_num, "^7ADMIN: ^2?sanctions^7, ^2?mute++^7, ^2?fixname^7");
		server.msg_to(say_num, "^7ADMIN: ^2?warnonsight^7, ^2?fixteams^7, ^2?reteam^7, ^2?spec^7");
	}
	else if(cmd == trans("!request") || cmd == trans("?request"))
	{
		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Request a map? Or any other suggestion..", true);
			server.msg_to(say_num, "^7ADMIN: ^3!request <request>");
			return true;
		}

		str request;
		sgl(iss >> std::ws, request);

		sofs ofs((katina.config_dir + "/requests.txt").c_str(), sofs::app);
		if(ofs << clients[say_num] << ": " << request << " [" << players[clients[say_num]] << "]"<< '\n')
		{
			server.msg_to(say_num, "^7ADMIN: "
					+  players[guid]
					           + "^3, your request has been logged.", true);
		}
	}
	else if(cmd == trans("!sanctions") || cmd == trans("?sanctions"))
	{
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3manage sanctions.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!sanctions = list all sanctions.");
			server.msg_to(say_num, "^7ADMIN: ^3!sanctions <num> = list sanctions for player.");
			return true;
		}
	}
	else if(cmd == trans("!fixteams") || cmd == trans("?fixteams"))
	{
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Shuffle the players to even the teams.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!fixteams");
			return true;
		}

		server.cp("^3Fixing Teams");

		fixteams();
	}
	else if(cmd == trans("!mute++") || cmd == trans("?mute++"))
	{
		// !mute++ <num> <duration>? <reason>?
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Keep a player muted for the specified duration.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!mute++ <num> <duration>? <reason>?");
			server.msg_to(say_num, "^7ADMIN: ^3        <duration> = N(s|m|h|d|w) [eg, 5w = 5 weeks]");
			server.msg_to(say_num, "^7ADMIN: ^3!mute++ remove = remove mute");
			return true;
		}

		siz num = siz(-1);
		str duration = "5m";
		str reason;

		sgl(iss >> num >> duration >> std::ws, reason);

		if(!check_slot(num))
			return true;

		if(duration == "remove")
		{
			remove_sanctions(clients[num], S_MUTEPP);
			if(un_mutepp(num))
			{
				server.msg_to(num, "^7ADMIN: ^3Removed mute from: ^2" + players[clients[num]], true);
				if(num != say_num)
					server.msg_to(say_num, "^7ADMIN: ^3Removed mute from: ^2" + players[clients[num]], true);
			}
			return true;
		}

		sanction s;
		s.type = S_MUTEPP;
		s.guid = clients[num];
		s.expires = parse_duration(duration, 5 * 60);

		if(mutepp(num))
			s.applied = true;

		sanctions.push_back(s);
		save_sanctions();
	}
	else if(cmd == trans("!fixname") || cmd == trans("?fixname"))
	{
		// !fixname <slot> <name>
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Force a player to have a specific name.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!fixname <num> <name>");
			server.msg_to(say_num, "^7ADMIN: ^3!fixname <num> remove");
			return true;
		}

		siz num = siz(-1);
		str name;

		sgl(iss >> num >> std::ws, name);

		if(!check_slot(num))
			return true;

		if(name == "remove")
		{
			remove_sanctions(clients[num], S_FIXNAME);
			server.msg_to(say_num, "^7ADMIN: ^3Removed fixed name from: ^2" + players[clients[num]], true);
			if(num != say_num)
				server.msg_to(num, "^7ADMIN: ^3Removed fixed name from: ^2" + players[clients[num]], true);
			return true;
		}

		sanction s;
		s.type = S_FIXNAME;
		s.guid = clients[num];
		s.expires = 0;
		s.params.push_back(name);

		if(fixname(num, name))
			s.applied = true;

		sanctions.push_back(s);
		save_sanctions();
	}
	else if(cmd == trans("!reteam") || cmd == trans("?reteam")
			|| cmd == trans("!spec") || cmd == trans("?spec"))
	{
		// !reteam <slot> r|b|s (red, blue or spec)
		if(!check_admin(guid))
			return true;

		bool spec = cmd.find(trans("!spec").substr(1)) == 1;

		if(cmd[0] == '?')
		{
			if(spec)
			{
				server.msg_to(say_num, "^7ADMIN: ^3Force a player to a spectate.", true);
				server.msg_to(say_num, "^7ADMIN: ^3!spec <num>");
				server.msg_to(say_num, "^7ADMIN: ^3!spec <num> remove");
			}
			else
			{
				server.msg_to(say_num, "^7ADMIN: ^3Force a player to a specific team (default to spec).", true);
				server.msg_to(say_num, "^7ADMIN: ^3!reteam <num> <r|g|b>");
				server.msg_to(say_num, "^7ADMIN: ^3!reteam <num> remove");
			}
			return true;
		}

		siz num = siz(-1);
		str team;

		sgl(iss >> num >> std::ws, team);

		if(!check_slot(num))
			return true;

		if(team == "remove")
		{
			remove_sanctions(clients[num], S_RETEAM);
			server.msg_to(say_num, "^7ADMIN: ^3Removed fixed team from: ^2" + players[clients[num]], true);
			if(num != say_num)
				server.msg_to(num, "^7ADMIN: ^3Removed fixed team from: ^2" + players[clients[num]], true);
			return true;
		}

		if(spec)
			team = "S";

		if(upper(team) != "R" || team != "B" || team != "S")
		{
			server.msg_to(say_num, "^7ADMIN: ^3Bad team. Needs to be: r|b|s", true);
			return true;
		}

		sanction s;
		s.type = S_RETEAM;
		s.guid = clients[num];
		s.expires = 0;
		s.params.push_back(team);

		if(reteam(num, team[0]))
			s.applied = true;

		sanctions.push_back(s);
		save_sanctions();
	}
	else if(cmd == trans("!warnonsight") || cmd == trans("?warnonsight"))
	{
		// TODO: contains crasher
		// !warnonsight <GUID> <reason>
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Warn a player next time they connect.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!warnonsight <GUID> <reason>");
			return true;
		}

		GUID guid;
		str reason;

		sgl(iss >> guid >> std::ws, reason);

		if(!guid)
		{
			server.msg_to(say_num, "^7ADMIN: ^1Error: ^3Bad GUID entered: ^2" + guid, true);
			return true;
		}

		if(trim(reason).empty())
		{
			server.msg_to(say_num, "^7ADMIN: ^1Error: ^3Must give reason to warn: ^2" + guid, true);
			return true;
		}

		if(reason == "remove")
		{
			remove_sanctions(guid, S_WARN_ON_SIGHT);
			server.msg_to(say_num, "^7ADMIN: ^3Removed warn-on-sight from: ^2" + guid, true);
			if(siz num = katina.getClientNr(guid) != siz(-1))
				server.msg_to(num, "^7ADMIN: ^3Removed warn-on-sight", true);

			return true;
		}

		sanction s;
		s.type = S_WARN_ON_SIGHT;
		s.guid = guid;
		s.expires = 0;
		s.reason = reason;
		//s.params.push_back(reason);

		sanctions.push_back(s);
		save_sanctions();
	}

	return true;
}

bool KatinaPluginAdmin::chat(siz min, siz sec, const str& text)
{
	if(!active)
		return true;

	plog("chat(" << text << ")");

	if(trim_copy(text) == "!fixteams")
		fixteams();

	return true;
}

bool KatinaPluginAdmin::shutdown_game(siz min, siz sec)
{
	if(!active)
		return true;

	for(siz_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
	{
		if((teams[i->second] == TEAM_R || teams[i->second] == TEAM_B) && time[i->first]) // playing, and timed
		{
			pbug("STOPPING TIMER FOR: " << players[i->second] << " after " << (katina.now - time[i->first]) << " seconds");
			secs[i->first] += (katina.now - time[i->first]);
			time[i->first] = 0; // stop
		}
	}

	return true;
}

bool KatinaPluginAdmin::exit(siz min, siz sec)
{
	if(!active)
		return true;
	return true;
}

bool KatinaPluginAdmin::unknown(siz min, siz sec, const str& cmd, const str& params)
{
	if(!active)
		return true;
	return true;
}

void KatinaPluginAdmin::close()
{
}

}} // katina::plugin
