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

#include "KatinaPluginAdmin.h"

#include <list>
#include <algorithm>
#include <functional>
#include <numeric>
#include <thread>
#include <future>

#include <ctime>

#include <katina/types.h>
#include <katina/log.h>
#include <katina/str.h>
#include <katina/codes.h>
#include <katina/time.h>

namespace katina { namespace plugin {

using namespace katina::log;
using namespace katina::types;
using namespace katina::time;
using namespace katina::string;

KATINA_PLUGIN_TYPE(KatinaPluginAdmin);
KATINA_PLUGIN_INFO("katina::admin", "Katina Admin", "0.1-dev");

sis& operator>>(sis& i, sanction& s)
{
	// 96BBD43E 4(S) 0
	i >> s.guid >> s.type;

	pbug_var(s.guid);
	pbug_var(s.type);

	str params;
	if(!sgl(sgl(i, params, '('), params, ')'))
	{
		plog("ERROR parsing sanction, missing parameters");
		i.setstate(std::ios::failbit);
		return i;
	}

	pbug_var(params);

	str param;
	s.params.clear();
	siss iss(params);
	while(iss >> param)
	{
		pbug_var(param);
		s.params.push_back(param);
	}

	if(!(i >> s.expires))
	{
		plog("ERROR parsing sanction expires date");
		return i;
	}

	sgl(i >> std::ws, s.reason);
	i.clear(); // optional reason

	return i;
}

sos& operator<<(sos& o, const sanction& s)
{
	o << s.guid << ' ' << s.type << "(";
	str sep;
	for(const str& param: s.params)
		{ o << sep << param; sep = " "; }
	o << ")";
	o << ' ' << s.expires << ' ' << s.reason;

	return o;
}

KatinaPluginAdmin::KatinaPluginAdmin(Katina& katina)
: KatinaPlugin(katina)
, mapname(katina.get_mapname())
, clients(katina.getClients())
, players(katina.getPlayers())
, teams(katina.getTeams())
, server(katina.server)
, active(true)
, total_kills(0)
, total_caps(0)
, policy(policy_t::FT_EVEN_SCATTER_DB)
//, spamkill_warn(3)
, spamkill_spams(5)
, spamkill_period(10)
, spamkill_mute(60)
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

bool KatinaPluginAdmin::load_sanctions()
{
	sifs ifs((katina.get_config_dir() + "/sanctions.dat").c_str());

	if(!ifs)
	{
		plog("ERROR: can not open sanctions file.");
		return false;
	}

	siz n = 0;
	str line;
	sanction s;
	while(sgl(ifs, line))
	{
		++n;
		if(trim(line).empty())
			continue;
		siss iss(line);
		if(!(iss >> s))
		{
			plog("ERROR: parsing sanction file: [" << n << "] " << line);
			return false;
		}

		if(s.expires && s.expires < katina.now)
			continue;

		str expires = s.expires ? ctime(&s.expires) : "PERMANENT";

		plog("SANCTION LOAD: [" << expires << ": " << s.type << "] " << katina.getPlayerName(s.guid));
		sanctions.push_back(s);
	}

	return true;
}

bool KatinaPluginAdmin::save_sanctions()
{
	sofs ofs((katina.get_config_dir() + "/sanctions.dat").c_str());

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
	, S_VOTEBAN
	, S_MAPBAN
};

struct player
{
	siz skill;
	slot num;

	player(siz skill, slot num): skill(skill), num(num) {}

	bool operator<(const player& p) const { return skill < p.skill; }
	bool operator==(const player& p) const { return skill == p.skill; }
};

typedef std::list<player> player_lst;
typedef std::vector<player> player_vec;

bool KatinaPluginAdmin::fair()
{
	pbug("-------------------------------------------");
	pbug("FAIR: policy: " << policy);
	pbug("-------------------------------------------");
	if(policy == policy_t::FT_NONE)
	{
		server.msg_to_all(katina.get_name() + "^7: ^3 FAIR is turned off");
		return true;
	}
//	siz_mmap rank; // skill -> slot
	player_vec rank;
	siz skill_r = 0;
	siz skill_b = 0;
	siz r = 0;
	siz b = 0;

	server.cp("^3Fixing Teams");

	double cap_factor = total_caps ? total_kills / total_caps : 1.0;
	bug_var(cap_factor);
	bug_var(clients.size());
	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
	{
//		if(i->second.is_bot())
//			continue;

		bug_var(i->first);
		bug_var(i->second);
		bug_var(katina.getTeam(i->second));
		 // 1 = red, 2 = blue, 3 = spec
		if(katina.getTeam(i->second) != TEAM_R && katina.getTeam(i->second) != TEAM_B)
			continue;

		pbug("FAIR:  slot: " << i->first);
		pbug("FAIR:  name: " << katina.getPlayerName(i->first));

		siz skill, fph,cph;
		if(stats && policy == policy_t::FT_EVEN_SCATTER_DB)
		{
			// get skill from database
			str res = stats->api("get_skill " + str(i->second) + " " + mapname);
			if(!res.find("ERROR"))
				continue;
			skill = to<siz>(res);
		}
		else
		{
			// accumulate time but keep timer running
			if(time[i->first])
			{
				secs[i->first] += (katina.now - time[i->first]);
				time[i->first] = katina.now;
			}

			fph = secs[i->first] ? (kills[i->first] * 60 * 60) / secs[i->first] : 0;
			cph = secs[i->first] ? (caps[i->first] * 60 * 60) / secs[i->first] : 0;

			pbug("FAIR: kills: " << kills[i->first]);
			pbug("FAIR:  caps: " << caps[i->first]);
			pbug("FAIR:  secs: " << secs[i->first]);
			pbug("FAIR:   fph: " << fph);
			pbug("FAIR:   cph: " << cph);

			skill = sqrt(pow(fph, 2) + pow(cph * cap_factor, 2));
		}

		pbug("FAIR: skill: " << skill);

		if(katina.getTeam(i->second) == TEAM_R)
			{ skill_r += skill; ++r; }
		else
			{ skill_b += skill; ++b; }

//		rank.insert(siz_mmap_pair(skill, i->first));
		rank.push_back(player(skill, i->first));
	}

	if(rank.size() < 3)
		return true;

	std::sort(rank.begin(), rank.end());

	pbug("-------------------------------------------");

	if(policy == policy_t::FT_EVEN_SCATTER || policy == policy_t::FT_EVEN_SCATTER_DB)
	{
		siz team = (rand() % 2) + 1;
		bug_var(team);
		for(const player&  i: rank)
		{
			pbug("FAIR: putting: " << i.num << " [" << i.skill << "] "
					<< "on team " << str(team == 1 ? "r" : "b"));

			if(katina.getTeam(i.num) != team)
				if(!server.command("!putteam " + to_string(i.num) + " " + str(team == 1 ? "r" : "b")))
					server.command("!putteam " + to_string(i.num) + " " + str(team == 1 ? "r" : "b")); // one retry
			team = team == 1 ? 2 : 1;
		}
	}
	else if(policy == policy_t::FT_BEST_PERMUTATION)
	{
		player_vec best_rank;
		siz best_delta = siz(-1);

		if(rank.size() & 1) // odd
		{
			siz total = 0;
			for(const player& p: rank)
				total += p.skill;

			rank.push_back(player(total / rank.size(), slot::bad));
		}

		std::sort(rank.begin(), rank.end());

		siz start_team = (rand() % 2) + 1;
		siz delta, team;
		do
		{
			siz tot[2] = {0, 0};
			team = start_team;
			pbug_var(team);

			str bug_sep;
			soss bug_out;

			for(siz i = 0; i < rank.size(); ++i)
			{
				bug_out << bug_sep << "{" << rank[i].num << ", " << rank[i].skill << "}";
				bug_sep = " ";
				tot[team - 1] += rank[i].skill;
				team = team == 1 ? 2 : 1;
			}

			pbug_var(bug_out.str());

			delta = tot[0] > tot[1] ? tot[0] - tot[1] : tot[1] - tot[0];

			if(delta < best_delta)
			{
				best_rank = rank;
				best_delta = delta;
			}
		}
		while(delta && std::next_permutation(rank.begin(), rank.end()));

		team = start_team;
		pbug_var(team);
		for(siz i = 0; i < best_rank.size(); ++i)
		{
			pbug("FAIR: putting: " << best_rank[i].num << " [" << best_rank[i].skill << "] "
					<< "on team " << str(team == 1 ? "r" : "b"));

			if(katina.getTeam(best_rank[i].num) != team)
				if(!server.command("!putteam " + to_string(best_rank[i].num) + " " + str(team == 1 ? "r" : "b")))
					server.command("!putteam " + to_string(best_rank[i].num) + " " + str(team == 1 ? "r" : "b")); // one retry
			team = team == 1 ? 2 : 1;
		}
	}
	else if(policy == policy_t::FT_NEAREST_DIFFERENCE)
	{
	}

	return true;
}

bool KatinaPluginAdmin::fixteams()
{
	pbug("-------------------------------------------");
	pbug("FIXTEAMS: policy: " << policy);
	pbug("-------------------------------------------");
	if(policy == policy_t::FT_NONE)
	{
		server.msg_to_all(katina.get_name() + "^7: ^3 FIXTEAMS is turned off");
		return true;
	}
//	siz_mmap rank; // skill -> slot
	player_vec rank;
	siz skill_r = 0;
	siz skill_b = 0;
	siz r = 0;
	siz b = 0;

	server.cp("^3Fixing Teams");

	double cap_factor = total_caps ? total_kills / total_caps : 1.0;
	bug_var(cap_factor);
	bug_var(clients.size());
	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
	{
//		if(i->second.is_bot())
//			continue;

		bug_var(i->first);
		bug_var(i->second);
		bug_var(katina.getTeam(i->second));
		 // 1 = red, 2 = blue, 3 = spec
		if(katina.getTeam(i->second) != TEAM_R && katina.getTeam(i->second) != TEAM_B)
			continue;

		pbug("FIXTEAMS:  slot: " << i->first);
		pbug("FIXTEAMS:  name: " << katina.getPlayerName(i->first));

		siz skill, fph,cph;
		if(stats && policy == policy_t::FT_EVEN_SCATTER_DB)
		{
			// get skill from database
			str res = stats->api("get_skill " + str(i->second) + " " + mapname);
			if(!res.find("ERROR"))
				continue;
			skill = to<siz>(res);
		}
		else
		{
			// accumulate time but keep timer running
			if(time[i->first])
			{
				secs[i->first] += (katina.now - time[i->first]);
				time[i->first] = katina.now;
			}

			fph = secs[i->first] ? (kills[i->first] * 60 * 60) / secs[i->first] : 0;
			cph = secs[i->first] ? (caps[i->first] * 60 * 60) / secs[i->first] : 0;

			pbug("FIXTEAMS: kills: " << kills[i->first]);
			pbug("FIXTEAMS:  caps: " << caps[i->first]);
			pbug("FIXTEAMS:  secs: " << secs[i->first]);
			pbug("FIXTEAMS:   fph: " << fph);
			pbug("FIXTEAMS:   cph: " << cph);

			skill = sqrt(pow(fph, 2) + pow(cph * cap_factor, 2));
		}

		pbug("FIXTEAMS: skill: " << skill);

		if(katina.getTeam(i->second) == TEAM_R)
			{ skill_r += skill; ++r; }
		else
			{ skill_b += skill; ++b; }

//		rank.insert(siz_mmap_pair(skill, i->first));
		rank.push_back(player(skill, i->first));
	}

	if(rank.size() < 3)
		return true;

	std::sort(rank.begin(), rank.end());

	pbug("-------------------------------------------");

	if(policy == policy_t::FT_EVEN_SCATTER || policy == policy_t::FT_EVEN_SCATTER_DB)
	{
		siz team = (rand() % 2) + 1;
		bug_var(team);
		for(const player&  i: rank)
		{
			pbug("FIXTEAMS: putting: " << i.num << " [" << i.skill << "] "
					<< "on team " << str(team == 1 ? "r" : "b"));

			if(katina.getTeam(i.num) != team)
				if(!server.command("!putteam " + to_string(i.num) + " " + str(team == 1 ? "r" : "b")))
					server.command("!putteam " + to_string(i.num) + " " + str(team == 1 ? "r" : "b")); // one retry
			team = team == 1 ? 2 : 1;
		}
	}
	else if(policy == policy_t::FT_BEST_PERMUTATION)
	{
		player_vec best_rank;
		siz best_delta = siz(-1);

		if(rank.size() & 1) // odd
		{
			siz total = 0;
			for(const player& p: rank)
				total += p.skill;

			rank.push_back(player(total / rank.size(), slot::bad));
		}

		std::sort(rank.begin(), rank.end());

		siz start_team = (rand() % 2) + 1;
		siz delta, team;
		do
		{
			siz tot[2] = {0, 0};
			team = start_team;
			pbug_var(team);

			str bug_sep;
			soss bug_out;

			for(siz i = 0; i < rank.size(); ++i)
			{
				bug_out << bug_sep << "{" << rank[i].num << ", " << rank[i].skill << "}";
				bug_sep = " ";
				tot[team - 1] += rank[i].skill;
				team = team == 1 ? 2 : 1;
			}

			pbug_var(bug_out.str());

			delta = tot[0] > tot[1] ? tot[0] - tot[1] : tot[1] - tot[0];

			if(delta < best_delta)
			{
				best_rank = rank;
				best_delta = delta;
			}
		}
		while(delta && std::next_permutation(rank.begin(), rank.end()));

		team = start_team;
		pbug_var(team);
		for(siz i = 0; i < best_rank.size(); ++i)
		{
			pbug("FIXTEAMS: putting: " << best_rank[i].num << " [" << best_rank[i].skill << "] "
					<< "on team " << str(team == 1 ? "r" : "b"));

			if(katina.getTeam(best_rank[i].num) != team)
				if(!server.command("!putteam " + to_string(best_rank[i].num) + " " + str(team == 1 ? "r" : "b")))
					server.command("!putteam " + to_string(best_rank[i].num) + " " + str(team == 1 ? "r" : "b")); // one retry
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

bool KatinaPluginAdmin::mutepp(slot num)
{
	str reply;
	server.command("!mute " + to_string(num), reply);
	// ����      broadcast: print "^3!mute: ^7^1S^2oo^3K^5ee^7 has been muted by ^7console\n"
	if(reply.find("broadcast: print") != str::npos
	&& reply.find("has been muted by") != str::npos)
		return true;
	return false;
}

bool KatinaPluginAdmin::fixname(slot num, const str& name)
{
	str reply;
	server.command("!rename " + to_string(num) + " " + name, reply);
	// ����      broadcast: print "^1S^2oo^3K^5ee^7 renamed to wibble\n"
	if(reply.find("broadcast: print") != str::npos
	&& reply.find("renamed to") != str::npos)
		return true;
	return false;
}

bool KatinaPluginAdmin::warn_on_sight(slot num, const str& reason)
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

bool KatinaPluginAdmin::reteam(slot num, const char team)
{
	plog("RETEAM CHECK: " << katina.getPlayerName(num) << " (" << str(num) << ")" << " to team: " << team);
	if(clients.find(num) == clients.end())
	{
		plog("ERROR: can't find client num: " << num);
		return true;
	}
	if(teams.find(katina.getClientGuid(num)) == teams.end())
	{
		plog("ERROR: can't find team guid: " << katina.getClientGuid(num));
		return true;
	}

	if(katina.getTeam(num) == char_to_team(team))
		return true;

	str reply;
	if(!server.command("!putteam " + to_string(num) + " " + (char)tolower(team), reply))
		if(!server.command("!putteam " + to_string(num) + " " + (char)tolower(team), reply))
			server.command("!putteam " + to_string(num) + " " + (char)tolower(team), reply);

	// ����
	// ����      ^/warn: no connected player by that name or slot #
	if(reply.find("!putteam: no connected player by that name or slot #") != str::npos)
	{
		plog("RETEAM FAIL : " << trim(reply));
		return false;
	}

	if(reply.find("!putteam: unknown team") != str::npos)
	{
		plog("RETEAM FAIL : " << trim(reply));
		return false;
	}

	plog("RETEAM ACTED: " << katina.getPlayerName(num) << " (" << str(num) << ")" << " to team: " << team);

	return true;
}

bool KatinaPluginAdmin::open()
{
	bug_func();

	stats = katina.get_plugin("katina::stats", "0.1");
	playerdb = katina.get_plugin("katina::playerdb", "0.1");

	// remote.irc.client: insecure 127.0.0.1:7334 #zimsnew(*)
	if(katina.has("admin.alert.irc.client"))
		if((irc = RemoteClient::create(katina, katina.get("admin.alert.irc.client"))))
			irc->on();

	plog("Adding var events");
	katina.add_var_event(this, "admin.active", active, false);
	katina.add_var_event(this, "admin.clientkick.protect", protect_admins, false);
	katina.add_var_event(this, "admin.fixteams.policy", policy, policy_t::FT_NONE);
	katina.add_var_event(this, "admin.detect.pushing", do_detect_pushing, false);

	// allows spamkill_spams / spamkill_period else !mute for spamkill_mute
	katina.add_var_event(this, "admin.spamkill.active", spamkill_active, false);
	katina.add_var_event(this, "admin.spamkill.spams", spamkill_spams, siz(2));
	katina.add_var_event(this, "admin.spamkill.period", spamkill_period, siz(6));
	katina.add_var_event(this, "admin.spamkill.mute", spamkill_mute, siz(60));
	//katina.add_var_event(this, "admin.active", spamkill_warn, 20); // not implemented

	plog("Adding log events");
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_CONNECT_INFO);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);
	katina.add_log_event(this, CLIENT_SWITCH_TEAM);
	katina.add_log_event(this, LOG_CALLVOTE);
	katina.add_log_event(this, KILL);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, CHAT);
	katina.add_log_event(this, SHUTDOWN_GAME);

	plog("Loading sanctions");
	load_sanctions();

// TODO: how to apply sanctions on startup?
//	slot num;
//	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end(); ++s)
//	{
//		if((num = katina.getClientSlot(s->guid)) == slot::bad)
//			continue;
//
//		if(players.find(s->guid) == players.end())
//			continue;
//
//		// !mute++
//		if(s->type == S_MUTEPP)
//			if(mutepp(num))
//				s->applied = true;
//		// !fixname
//		if(s->type == S_FIXNAME && !s->params.empty()
//		&& s->params[0] == katina.getPlayerName(s->guid))
//			if(fixname(num, s->params[0]))
//				s->applied = true;
//	}

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

void KatinaPluginAdmin::heartbeat(siz min, siz sec)
{
	// once a minute only
	static siz last_min = min;

	if(last_min == min)
		return;

	last_min = min;

	pbug("SPAMKILL HEARTBEAT check:");

	slot_lst eraseures;
	for(mute_map_vt m: mutes)
		if((m.second + spamkill_mute) < katina.now)
			eraseures.push_back(m.first);

	for(const slot& num: eraseures)
	{
		const GUID& guid = katina.getClientGuid(num);

		str name = katina.getPlayerName(guid);

		if(guid == null_guid || name.empty()) // disconnected?
			mutes.erase(num);
		else
		{
			pbug("SPAMKILL DEACTIVATED FOR: " << name << " [" << guid << "] (" << str(num) << ")");

//			if(server.command("!unmute " + str(num)))
				mutes.erase(num);
		}
	}
}

bool KatinaPluginAdmin::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;

	// !muted become unstuck after every game
	slot num;
	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end();)
	{
		// expired?
		if(s->expires && s->expires >= katina.now)
			{ s = sanctions.erase(s); save_sanctions(); continue; }

		if((num = katina.getClientSlot(s->guid)) == slot::bad) // not connected
			{ ++s; continue; }

		if(players.find(s->guid) == players.end())
			{ ++s; continue; }

		if(s->type == S_MUTEPP)
			if(mutepp(num))
				s->applied = true;

		if(s->type == S_RETEAM && !s->params.empty() && !s->params[0].empty())
			if(reteam(num, s->params[0][0]))
				s->applied = true;

		if(s->type == S_MAPBAN && !s->params.empty() && s->params[0] == katina.get_mapname())
			if(reteam(num, 's'))
				s->applied = true;
		++s;
	}

	// fixteams
	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
	{
		if((katina.getTeam(i->second) == TEAM_R || katina.getTeam(i->second) == TEAM_B))
		{
			pbug("STARTING TIMER FOR: " << katina.getPlayerName(i->second) << " [" << katina.now << "]");
			time[i->first] = katina.now;
		}
	}
	return true;
}

bool KatinaPluginAdmin::warmup(siz min, siz sec)
{
	if(!active)
		return true;

	// TODO: consider !fixteams on warmup

	// stop all timers
	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
		time[i->first] = 0;

	return true;
}

bool KatinaPluginAdmin::client_connect(siz min, siz sec, slot num)
{
	if(!active)
		return true;
	kills[num] = 0;
	caps[num] = 0;
	secs[num] = 0;
	time[num] = 0;
	return true;
}

bool KatinaPluginAdmin::client_connect_info(siz min, siz sec, slot num, const GUID& guid, const str& ip)
{
	if(!active)
		return true;

	for(str_vec_citer i = katina.get_vec("admin.ban.ip").begin(); i != katina.get_vec("admin.ban.ip").end(); ++i)
		if(!ip.find(*i)) // left substring match
			server.command("!ban " + to_string(num) + "AUTO BAN IP: " + *i);

	for(str_vec_citer i = katina.get_vec("admin.ban.guid").begin(); i != katina.get_vec("admin.ban.guid").end(); ++i)
		if(guid == GUID(*i))
			server.command("!ban " + to_string(num) + "AUTO BAN GUID: " + *i);

	for(str_vec_citer i = katina.get_vec("admin.alert.ip").begin(); i != katina.get_vec("admin.alert.ip").end(); ++i)
		if(!ip.find(*i)) // left substring match
			if(irc)
				irc->chat('a', "^1!^3ALERT^1! ^7(^2" + str(guid) + "^7) " + katina.getPlayerName(guid) + " ^3has joined the server");

	for(str_vec_citer i = katina.get_vec("admin.alert.guid").begin(); i != katina.get_vec("admin.alert.guid").end(); ++i)
		if(guid == GUID(*i))
			if(irc)
				irc->chat('a', "^1!^3ALERT^1! ^7(^2" + str(guid) + "^7) " + katina.getPlayerName(guid) + " ^3has joined the server");

	return true;
}

bool KatinaPluginAdmin::client_begin(siz min, siz sec, slot num)
{
	if(!active)
		return true;
	return true;
}

bool KatinaPluginAdmin::client_disconnect(siz min, siz sec, slot num)
{
	if(!active)
		return true;

	{ // !fixteams
		kills[num] = 0;
		caps[num] = 0;
		secs[num] = 0;
		time[num] = 0;
	}

	return true;
}

bool KatinaPluginAdmin::client_userinfo_changed(siz min, siz sec, slot num, siz team
		, const GUID& guid, const str& name, siz hc)
{
	if(!active)
		return true;

	if(guid.is_bot() || !guid.is_connected())
		return true;

//	bug("SANCTION CHECK FOR: " << guid);
	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end();)
	{
//		bug("SANCTION FOUND: " << s->type);
		if(s->expires && s->expires >= katina.now)
			{ s = sanctions.erase(s); save_sanctions(); continue; }

//		bug("SANCTION CURRENT: " << (s->expires ? ctime(&s->expires):"PERMANENT"));
		if(s->guid != guid)
			{ ++s; continue; }

//		bug("SANCTION APPLICABLE: " << s->guid);
		if(s->type == S_FIXNAME && !s->params.empty() && name != s->params[0])
		{
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
		else if(s->type == S_MAPBAN && !s->params.empty() && s->params[0] == katina.get_mapname())
		{
			if(reteam(num, 's'))
				s->applied = true;
			++s;
		}
		else
			++s;
	}

	return true;
}

bool KatinaPluginAdmin::client_switch_team(siz min, siz sec, slot num, siz teamBefore, siz teamNow)
{
	if(!active)
		return true;

	//plog("client_switch_team(" << num << ", " << teamBefore << ", " << teamNow << ")");

	if((teamNow == TEAM_R || teamNow == TEAM_B) && !time[num])
	{
		//pbug("STARTING TIMER FOR: " << katina.getPlayerName(num) << " [" << katina.now << "]");
		time[num] = katina.now; // start timer if not running
	}
	else if(teamNow == TEAM_S && time[num])
	{
		//pbug("STOPPING TIMER FOR: " << katina.getPlayerName(num) << " after " << (katina.now - time[num]) << " seconds");
		secs[num] += (katina.now - time[num]);
		time[num] = 0; // stop timer if running
	}

	return true;
}

str secs_to_dhms(siz secs)
{
	siz s = secs % 60;
	siz m = (secs / 60) % 60;
	siz h = ((secs / 60) / 60) % 24;
	siz d = (((secs / 60) / 60) / 24) % 365;

	str sep;
	soss oss;

	if(d)
		{ oss << sep << d << "d"; sep = " "; }
	if(h|d)
		{ oss << sep << h << "h"; sep = " "; }
	if(m|h|d)
		{ oss << sep << m << "m"; sep = " "; }

	oss << sep << s << "s";

	return oss.str();

//	 total_seconds = 939000000
//
//	seconds = total_seconds % 60
//	total_minutes = total_seconds / 60
//	minutes = total_minutes % 60
//	total_hours = total_minutes / 60
//	hours = total_hours % 24
//	total_days = total_hours / 24
//	days = total_days % 365
//	years = total_days / 365
}

bool KatinaPluginAdmin::votekill(const str& reason)
{
	pbug("VOTEKILL ACTIVATED: " << reason);
	if(!server.command("!cancelvote"))
		return false;
	server.msg_to_all(reason);
	plog("VOTEKILL ACTIVATED: " << reason);
	return true;
}

bool KatinaPluginAdmin::callvote(siz min, siz sec, slot num, const str& type, const str& info)
{
	plog("CALLVOTE: " << num << ", " << katina.getPlayerName(num) << " " << type << " " << info);

	for(const str& v: katina.get_vec("admin.ban.vote"))
	{
		str t;
		str i;
		str reason;
		siss iss(v);

		if(!(iss >> t >> std::ws >> i))
		{
			plog("ERROR: parsing admin.ban.vote: " << v);
			continue;
		}

		if(!sgl(iss >> std::ws, reason))
			plog("WARN: missing reason from admin.ban.vote: " << v);

		if(!trim(reason).empty())
			reason = " ^7[^3" + reason + "^7]";

		if(t == type && i == info)
		{
			std::async(std::launch::async, [&]
			{
				thread_sleep_millis(2000);
				votekill(katina.get_name() + "^1: ^5This vote has been disallowed: " + reason);
				plog("VOTEKILL: admin.ban.vote: " << v);
			});
		}
	}

	if(!katina.check_slot(num))
		return true;

	for(const sanction& s: sanctions)
		if((!s.expires || s.expires < katina.now) && s.type == S_VOTEBAN && s.guid == katina.getClientGuid(num))
		{
			std::async(std::launch::async, [&]
			{
				thread_sleep_millis(2000);
				votekill(katina.get_name() + "^1: " + katina.getPlayerName(num) + " is banned from voting for "
				+ secs_to_dhms(s.expires - katina.now));
				plog("VOTEKILL: prevented " << katina.getClientGuid(num) << ": banned: " << s.reason);
			});
		}

	if(protect_admins)
	{
		if(type == "clientkick" && katina.is_admin(katina.getClientGuid(to<slot>(info))))
		{
			std::async(std::launch::async, [&]
			{
				thread_sleep_millis(2000);
				votekill(katina.get_name() + "^1: ^7[^3NOT ALLOWED TO KICK ADMINS^7]");
				plog("VOTEKILL: admin protection for: " << katina.getPlayerName(to<slot>(info)));
			});
		}
	}
	return true;
}

bool KatinaPluginAdmin::kill(siz min, siz sec, slot killer, slot killed, siz weap)
{
	if(!active)
		return true;

//	if(do_detect_pushing && killer == world_slot && weap == MOD_TRIGGER_HURT)
//	{
//		pbug("DETECTING HOSTILE PUSHES:");
//		bool warned = false;
//		siz suspicious = 0;
//		for(push_lst_iter p = pushes.begin(); p != pushes.end();)
//		{
//			pbug(katina.getPlayerName(p->pusher) << " push-killed " << katina.getPlayerName(pushed));
//			if(katina.now - p->when > 3) // 3 seconds
//			{
//				p = pushes.erase(p);
//				continue;
//			}
//
//			if(p->pushed != killed)
//			{
//				++p;
//				continue;
//			}
//
//			if(warned)
//			{
//				p = pushes.erase(p);
//				continue;
//			}
//
//			if(++suspicious < 3)
//			{
//				++p;
//				continue;
//			}
//
//			if(irc && irc->chat('a', "^3!^1ALERT^3! ^7" + katina.get("admin.alert.irc.admins")
//				+ " ^3possible pusher: " + katina.getPlayerName(p->pusher)))
//			{
//				plog("HOSTILE PUSHING DETECTED!!!!");
////				server.msg_to(p->pusher, katina.get_name() + "^7: "
////						+   katina.getPlayerName(p->pusher)
////								   + "^3, your ^7PUSHES ^3are suspicious, ^7ADMIN ^3alerted", true);
//				p = pushes.erase(p);
//				warned = true;
//			}
//		}
//		return true;
//	}

	 // !fixteams
	if(time[killer]) // is the timer running?
		++kills[killer];
	++total_kills;
	return true;
}

bool KatinaPluginAdmin::push(siz min, siz sec, slot pusher, slot pushed)
{
//	push_evt p;
//	p.when = katina.now;
//	p.pusher = pusher;
//	p.pushed = pushed;
//	pushes.push_back(p);
	return true;
}

bool KatinaPluginAdmin::ctf(siz min, siz sec, slot num, siz team, siz act)
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

		sifs ifs((katina.get_config_dir() + "/"
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
		plog("WARN: Admin attempt by non admin player: [" << guid << "] "
			<< katina.getPlayerName(guid));
		return false;
	}
	return true;
}

bool KatinaPluginAdmin::check_slot(slot num)
{
	if(katina.check_slot(num))
		return true;
	plog("WARN: Unknown client number: " << num);
	server.chat_nobeep("^7!ADMIN: ^3Unknown client number: ^7" + to_string(num));
	return false;
}

bool parse_duration(const str& duration, std::chrono::seconds& secs)
{
	siz s;

	// 30 w|d|h|m|s

	siss iss(duration);

	str units = "m";
	if(!(iss >> s >> std::ws >> units))
		return false;

	if(units == "s")
		s *= 1;
	else if (units == "m")
		s *= 60;
	else if (units == "h")
		s *= 60 * 60;
	else if (units == "d")
		s *= 60 * 60 * 24;
	else if (units == "w")
		s *= 60 * 60 * 24 * 7;

	secs = std::chrono::seconds(s);

	return true;
}

std::time_t KatinaPluginAdmin::duration_to_time(const str& duration, const std::chrono::seconds& dflt)
{
	std::chrono::seconds secs;

	if(!parse_duration(duration, secs))
	{
		plog("ERROR: parsing duration: " << duration);
		return katina.now + dflt.count();
	}

	return katina.now + secs.count();
}

bool KatinaPluginAdmin::remove_sanctions(const GUID& guid, siz type)
{
	if(guid == null_guid)
	{
		plog("ERROR: null guid");
		return false;
	}
	bool done = false;
	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end();)
		if(s->guid == guid && s->type == type)
			{ s = sanctions.erase(s); done = true; }
		else
			++s;

	if(!done)
		return false;

	return save_sanctions();
}

bool KatinaPluginAdmin::spamkill(slot num)
{
	if(!spamkill_active)
		return true;

	const GUID& guid = katina.getClientGuid(num);

	if(guid == null_guid)
	{
		plog("ERROR: Unexpected null guid for slot: " << num);
		return false;
	}

	static const str STAGE_1 = "^3PLEASE STOP SPAMMING^7: ^2It distracts other players.";
	static const str STAGE_2 = "^3PLEASE STOP SPAMMING^7: ^2Automatic !mute system ready.";
	static const str STAGE_3 = "^3Your ^7SPAM ^3has triggered ^7auto-mute";

	const str name = katina.getPlayerName(guid);

	pbug("SPAMKILL ACTIVATED FOR: " << name << " [" << guid << "] (" << num << ")");

	str prefix = katina.get_name() + "^7: " + name + " ";

	if(spamkill_stage[num] == 1 /*&& server.msg_to(num, prefix + STAGE_1, true)*/)
	{
		pbug("SPAMKILL WARNING #1");
		++spamkill_stage[num];
		return true;
	}
	else if(spamkill_stage[num] == 2 /*&& server.msg_to(num, prefix + STAGE_2, true)*/)
	{
		pbug("SPAMKILL WARNING #2");
		++spamkill_stage[num];
		return true;
	}
	else if(spamkill_stage[num] > 2 /*&& server.command("!mute " + to_string(num))*/)
	{
		pbug("SPAMKILL MUTE");
//		server.msg_to(num, prefix + STAGE_3 + " ^3for ^7" + to_string(spamkill_mute) + " ^3seconds");
		mutes[num] = katina.now;
		spamkill_stage[num] = 0;
		return true;
	}

	return false;
}

void KatinaPluginAdmin::tell_perp(slot admin_num, slot perp_num, const str& msg)
{
	if(admin_num != slot::bad)
		server.msg_to(admin_num, "^7ADMIN: " + msg, true);
	if(perp_num != admin_num && perp_num != slot::bad)
		server.msg_to(perp_num, "^7ADMIN: " + msg, true);
}

// TODO: make these commands use katina.parse_slot_guid_name()
bool KatinaPluginAdmin::say(const siz min, const siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;

	plog("say(" << guid << ", " << text << ")");

	// !cmd <parans>

	slot say_num;
	if((say_num = katina.getClientSlot(guid)) == slot::bad)
	{
		plog("ERROR: Unable to get slot number from guid: " << guid);
		return true;
	}

	if(!check_slot(say_num))
		return true;

	// spamkill

	if(spamkill_active && !mutes.count(say_num))
	{
		spam s;
		s.num = say_num;
		s.when = katina.now;
//		bug_var(spams[text].size());
		spam_lst& list = spams[text];
		list.push_back(s);
//		bug_var(spams[text].size());

//		bug("<== SPAMCHECK ==>");

		siz count = 0;
		for(spam_lst_iter i = list.begin(); i != list.end();)
		{
			if(i->num != say_num)
				++i;
			else if(i->when + spamkill_period <= katina.now)
				i = list.erase(i);
			else if(++count <= spamkill_spams)
				++i;
			else if(!spamkill(say_num))
				++i;
			else
			{
				//bug("<== SPAM FOUND & KILLED ERASING ==>");
				for(spam_lst_iter i = list.begin(); i != list.end();)
				{
					(i->num != say_num && i->when + spamkill_period > katina.now) ? ++i : (i = list.erase(i));
					// erase all spams for this player and any other expired spams
//					if(i->num != say_num && i->when + spamkill_period > katina.now)
//						++i;
//					else
//						i = list.erase(i);
				}
				break;
			}
		}
	}
	// /spamkill

	siss iss(text);

	str cmd, params;

	if(!(iss >> cmd >> std::ws))
	{
//		plog("ERROR parsing admin command.");
		return true;
	}

	sgl(iss, params);

	iss.clear();
	iss.str(params);

	if(cmd == trans("!help") || cmd == trans("?help"))
	{
		server.msg_to(say_num, "^7ADMIN: ^2?request^7, ^2?alert^7", true);

		if(!check_admin(guid))
			return true;

		server.msg_to(say_num, "^7ADMIN: ^2?sanctions^7, ^2?mute++^7, ^2?fixname^7, ^2?voteban^7");
		server.msg_to(say_num, "^7ADMIN: ^2?warnonsight^7, ^2?fixteams^7, ^2?reteam^7, ^2?spec^7");
		server.msg_to(say_num, "^7ADMIN: ^2?mapban^7");
	}
	else if(cmd == trans("!alert") || cmd == trans("?alert"))
	{
		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Request an admin to attend the server.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!alert <reason>");
			return true;
		}

		const str_vec& banned_ips = katina.get_vec("admin.alert.banned.ip");
		const str_vec& banned_guids = katina.get_vec("admin.alert.banned.guid");

		str cmd = "guid_to_ip " + str(guid);
		str inip = playerdb ? playerdb->api(cmd):"";

		if(!inip.find("ERROR:"))
			plog("ERROR: calling playerdb::api(" << cmd);

		if(std::find_if(banned_ips.begin(), banned_ips.end(), [&](const str& ip){return !ip.find(inip);}) != banned_ips.end()
		|| std::find(banned_guids.begin(), banned_guids.end(), str(guid)) != banned_guids.end())
//		if( std::find(banned_guids.begin(), banned_guids.end(), str(guid)) != banned_guids.end())
		{
			server.msg_to(say_num, katina.get_name() + "^7: "
				+   katina.getPlayerName(guid)
						   + "^3, sorry, ^7!alert ^3disabled for misuse.", true);
			return true;
		}

		str request;
		sgl(iss >> std::ws, request);

		if(irc)
		{
			const str_vec& admins = katina.get_vec("admin.irc.alert");

			if(admins.empty())
			{
				server.msg_to(say_num, katina.get_name() + "^7: "
					+   katina.getPlayerName(guid)
							   + "^3, sorry, no admins configured.", true);
				return true;
			}

			soss msg;

			msg << " ^7!^1ALERT^7! ^3" + request + " ^7[" + katina.getPlayerName(guid) + "^7]";

			for(const str& admin: admins)
			{
				plog("ALERTING ADMIN: " << admin);
				irc->send("/msg " + admin + " " + oa_to_IRC(msg.str()));
			}

//			if(irc->chat('a', "^3!^1ALERT^3! ^7" + katina.get("admin.alert.irc.admins") + " ^3" + request))
//				server.msg_to(say_num, katina.get_name() + "^7: "
//						+   katina.getPlayerName(guid)
//								   + "^3, admin have been alerted.", true);
		}
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

		sofs ofs((katina.get_config_dir() + "/requests.txt").c_str(), sofs::app);
		if(ofs << guid << ": " << request << " [" << katina.getPlayerName(guid) << "]"<< '\n')
		{
			server.msg_to(say_num, "^7ADMIN: "
					+   katina.getPlayerName(guid)
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

		// !sanctions
		// !sanctions slot
		// !sanctions -d|del|delete idx
		// !sanctions -m|mod|modify idx [duration] [reason]
		//
		// slot: 0-64
		// idx: 0-sanctions.size() - 1
		// duration: num "s"|"m"|"h"|"d"|"w"

		slot num;
		if(!(iss >> cmd) || ((siss(cmd) >> num).eof()))
		{
			// list
		}
		else if(cmd == "-d" || cmd == "del" || cmd == "delete")
		{
			siz idx;
			if(!(iss >> idx))
			{
				plog("");
				return true;
			}
		}
		else if(cmd == "-m" || cmd == "mod" || cmd == "modify")
		{

		}
	}
	else if(cmd == trans("!fair") || cmd == trans("?fair"))
	{
		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Check the teams for fairness.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!fair");
			return true;
		}

		fair();
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

		slot num = slot::bad;
		str duration = "5m";
		str reason;

		sgl(iss >> num >> duration >> std::ws, reason);

		if(!check_slot(num))
			return true;

		if(duration == "remove")
		{
			if(remove_sanctions(katina.getClientGuid(num), S_MUTEPP) && un_mutepp(num))
				tell_perp(say_num, num, "^3Removed mute from: ^7" + katina.getPlayerName(num));
			return true;
		}

		sanction s;
		s.type = S_MUTEPP;
		s.guid = katina.getClientGuid(num);
		s.expires = duration_to_time(duration, minutes(5));
		s.reason = reason;

		if(s.guid == null_guid)
		{
			plog("ERROR: Null guid");
			return true;
		}

		if(mutepp(num))
			s.applied = true;

		sanctions.push_back(s);
		if(save_sanctions())
			plog("mute++ applied to " << s.guid << " " << katina.getPlayerName(guid) << " by " << katina.getPlayerName(say_num));
	}
	else if(cmd == trans("!voteban") || cmd == trans("?voteban"))
	{
		// !mute++ <num> <duration>? <reason>?
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Prevent a player from voting for the specified duration.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!voteban <num> <duration>? <reason>?");
			server.msg_to(say_num, "^7ADMIN: ^3        <duration> = N(s|m|h|d|w) [eg, 5w = 5 weeks]");
			server.msg_to(say_num, "^7ADMIN: ^3!voteban remove = remove ban");
			return true;
		}

		slot perp = slot::bad;
		str duration = "5m";
		str reason;

		sgl(iss >> perp >> duration >> std::ws, reason);

		if(!check_slot(perp))
			return true;

		if(duration == "remove")
		{
			if(remove_sanctions(katina.getClientGuid(perp), S_VOTEBAN))
				tell_perp(say_num, perp, "^3Removed vote ban from: ^2" + katina.getPlayerName(perp));
			return true;
		}

		sanction s;
		s.type = S_VOTEBAN;
		s.guid = katina.getClientGuid(perp);
		s.expires = duration_to_time(duration, minutes(10));
		s.reason = reason;

		if(s.guid == null_guid)
		{
			plog("ERROR: Null guid");
			return true;
		}

		sanctions.push_back(s);
		if(save_sanctions())
			tell_perp(say_num, perp, "^7" + katina.getPlayerName(s.guid) + "^3BANNED from voting by ^7" +  katina.getPlayerName(say_num));
	}
	else if(cmd == trans("!mapban") || cmd == trans("?mapban"))
	{
		// !mapban <num> <duration>? <reason>?
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Force a player to spec on a specific map for the specified duration.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!mapban <num> <duration>? <reason>?");
			server.msg_to(say_num, "^7ADMIN: ^3        <duration> = N(s|m|h|d|w) [eg, 5w = 5 weeks]");
			server.msg_to(say_num, "^7ADMIN: ^3!mapban <num> remove = remove ban");
			return true;
		}

		slot num = slot::bad;
		str duration = "20m";
		str reason;

		if(!(iss >> num >> std::ws))
		{
			plog("ERROR parsing !mapban command.");
			return true;
		}

		if(!check_slot(num))
			return true;

		sgl(iss >> duration >> std::ws, reason);

		if(duration == "remove")
		{
			if(remove_sanctions(katina.getClientGuid(num), S_MAPBAN))
				tell_perp(num, say_num, "^3Removed ^1map ban ^3from: ^7" + katina.getPlayerName(num));
			return true;
		}

		sanction s;
		s.type = S_MAPBAN;
		s.guid = katina.getClientGuid(num);
		s.expires = duration_to_time(duration, minutes(20));
		s.reason = reason;
		s.params.push_back(katina.get_mapname());

		if(s.guid == null_guid)
		{
			plog("ERROR: Null guid");
			return true;
		}

		sanctions.push_back(s);
		save_sanctions();

		if(reteam(num, 's'))
		{
			s.applied = true;
			tell_perp(say_num, num, "^3BANNED ^7" + katina.getPlayerName(num) + " ^3from ^1" + katina.get_mapname() + " ^3by ^7" +  katina.getPlayerName(say_num));
			tell_perp(say_num, num, "^3BANNED for ^7" + duration);
		}
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

		slot perp = slot::bad;
		str name;

		sgl(iss >> perp >> std::ws, name);

		if(!check_slot(perp))
			return true;

		if(name == "remove")
		{
			if(remove_sanctions(katina.getClientGuid(perp), S_FIXNAME))
				tell_perp(say_num, perp, "^7" + katina.getPlayerName(say_num) + " ^3Removed ^1fixed name^3 from: ^7" + katina.getPlayerName(perp));
			return true;
		}

		sanction s;
		s.type = S_FIXNAME;
		s.guid = katina.getClientGuid(perp);
		s.expires = 0; //duration_to_time("1h", hours(1));
		s.params.push_back(name);

		if(s.guid == null_guid)
		{
			plog("ERROR: Null guid");
			return true;
		}

		if(fixname(perp, name))
		{
			s.applied = true;
			tell_perp(say_num, perp, "^3FIXED NAME given to ^7" + katina.getPlayerName(s.guid) + " ^3by ^7" +  katina.getPlayerName(say_num));
		}

		sanctions.push_back(s);
		save_sanctions();
	}
	else if(cmd == trans("!reteam") || cmd == trans("?reteam") // working
			|| cmd == trans("!spec") || cmd == trans("?spec"))
	{
		// TODO: fix these bugs

//		1401672351 ^1S^2oo^3K^5ee^7: ^2!spec 2
//		1401672351 ^7ADMIN: ^3Bad team. Needs to be: r|b|s

//		1401672395 ^1S^2oo^3K^5ee^7: ^2!reteam 2 s
//		1401672395 ^7ADMIN: ^3Bad team. Needs to be: r|b|s


		// !reteam <slot> r|b|s (red, blue or spec)
		if(!check_admin(guid))
			return true;

		bool spec = (cmd == "!spec" || cmd == "?spec");

		pbug_var(spec);

		if(cmd[0] == '?')
		{
			if(spec)
			{
				server.msg_to(say_num, "^7ADMIN: ^3Force a player to a spectate.", true);
				server.msg_to(say_num, "^7ADMIN: ^3!spec <num> [<reason>]");
				server.msg_to(say_num, "^7ADMIN: ^3!spec <num> remove");
			}
			else
			{
				server.msg_to(say_num, "^7ADMIN: ^3Force a player to a specific team (default to spec).", true);
				server.msg_to(say_num, "^7ADMIN: ^3!reteam <num> <r|g|b> [<reason>]");
				server.msg_to(say_num, "^7ADMIN: ^3!reteam <num> remove");
			}
			return true;
		}

		slot perp = slot::bad;
		str team, reason;

		if(spec)
			sgl(iss >> perp >> std::ws, reason);
		else
			sgl(iss >> perp >> team >> std::ws, reason);

		if(spec)
			team = "S";

		pbug_var(perp);
		pbug_var(team);
		pbug_var(reason);

		trim(team);
		trim(reason);

		pbug_var(trim(reason));

		if(!check_slot(perp))
			return true;

		if((spec && reason == "remove") || team == "remove")
		{
			str sanct = spec ? "^1spec":"^1fixed team";
			if(remove_sanctions(katina.getClientGuid(perp), S_RETEAM))
				tell_perp(say_num, perp, "^7" + katina.getPlayerName(guid) + " ^3Removed " + sanct + " ^3from: ^7" + katina.getPlayerName(perp));
			return true;
		}

		if(upper(team) != "R" && team != "B" && team != "S")
		{
			server.msg_to(say_num, "^7ADMIN: ^3Bad team. Needs to be: r|b|s", true);
			return true;
		}

		sanction s;
		s.type = S_RETEAM;
		s.guid = katina.getClientGuid(perp);
		s.expires = duration_to_time("5m", minutes(5));
		s.params.push_back(team);
		s.reason = reason;

		if(s.guid == null_guid)
		{
			plog("ERROR: Null guid");
			return true;
		}

		sanctions.push_back(s);

		if(save_sanctions() && reteam(perp, team[0]))
		{
			s.applied = true;
			str sanct = spec ? "^3FORCED SPEC":"^3FORCED TEAM";
			tell_perp(say_num, perp, sanct + " ^3given to ^7" + katina.getPlayerName(s.guid) + " ^3by ^7" +  katina.getPlayerName(say_num));
		}
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

		GUID perp;
		str reason;

		sgl(iss >> perp >> std::ws, reason);

		if(!perp)
		{
			server.msg_to(say_num, "^7ADMIN: ^1Error: ^3Bad GUID entered: ^2" + str(perp), true);
			return true;
		}

		if(trim(reason).empty())
		{
			server.msg_to(say_num, "^7ADMIN: ^1Error: ^3Must give reason to warn: ^2" + str(perp), true);
			return true;
		}

		if(reason == "remove")
		{
			slot num = katina.getClientSlot(perp);
			if(remove_sanctions(perp, S_WARN_ON_SIGHT) && num != slot::bad)
				tell_perp(say_num, num, "^7" + katina.getPlayerName(perp) + " ^3Removed warn-on-sight from: ^7" + katina.getPlayerName(perp));
			return true;
		}

		sanction s;
		s.type = S_WARN_ON_SIGHT;
		s.guid = perp;
		s.expires = 0;
		s.reason = reason;
		//s.params.push_back(reason);

		sanctions.push_back(s);

		if(save_sanctions())
			server.msg_to(say_num, "^3Warning set for ^1" + str(s.guid), true);
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

	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
	{
		if((katina.getTeam(i->second) == TEAM_R || katina.getTeam(i->second) == TEAM_B) && time[i->first]) // playing, and timed
		{
			pbug("STOPPING TIMER FOR: " << katina.getPlayerName(i->second) << " after " << (katina.now - time[i->first]) << " seconds");
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
