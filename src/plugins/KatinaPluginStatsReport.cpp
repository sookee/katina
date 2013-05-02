
#include "KatinaPlugin.h"
#include "KatinaPluginStats.h"

#include "Database.h"
#include "GUID.h"

#include "types.h"
#include "log.h"
#include "KatinaPluginStatsReport.h"

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

const str HUD_FLAG_P = "⚑";
const str HUD_FLAG_DIE = "*";
const str HUD_FLAG_CAP = "Y";
const str HUD_FLAG_NONE = ".";
const str HUD_FLAG_RETURN = "^";

str hud_flag[2] = {HUD_FLAG_NONE, HUD_FLAG_NONE};

str get_hud(siz m, siz s, str hud_flag[2])
{
	soss oss;
	oss << "00[15" << (m < 10?"0":"") << m << "00:15" << (s < 10?"0":"") << s << " ";
	oss << "04" << hud_flag[FL_RED];
	oss << "02" << hud_flag[FL_BLUE];
	oss << "00]";
	return oss.str();
}

virtual bool KatinaPluginStatsReport::open(Katina& katina)
{
	if(!katina.has_plugin("katina::stats", "0.0"))
		return false;

	if(!(kpsp = katina_get_pligin_raw_ptr("katina::stats"))
		return false;

	skivvy.config(recs["skivvy.host"], to<siz>(recs["skivvy.port"]));

	skivvy.chat('*', "^3Stats Reporting System v^7" + get_version() + " - ^1ONLINE");

	return true;
}

virtual str KatinaPluginStatsReport::get_id() const
{
	return "katina::stats::report";
}

virtual str KatinaPluginStatsReport::get_name() const
{
	return "Stats Reporting";
}

virtual str KatinaPluginStatsReport::get_version() const
{
	return "0.1-dev";
}

virtual bool KatinaPluginStatsReport::exit(GameInfo& gi)
{
	if(!in_game)
		return true;
	in_game = false;

	skivvy.chat('*', "^3Game Over");

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

	std::multimap<double, str> skivvy_scores;

	soss oss;
	oss.str("");
	str sep;
	if(sk_cfg.stats_cols & skivvy_conf::RSC_TIME)
		{ oss << sep << "^3time "; sep = "^2|"; }
	if(sk_cfg.stats_cols & skivvy_conf::RSC_FPH)
		{ oss << sep << "^3fph"; sep = "^2|"; }
	if(sk_cfg.stats_cols & skivvy_conf::RSC_TIME)
		{ oss << sep << "^3cph"; sep = "^2|"; }
	if(sk_cfg.stats_cols & skivvy_conf::RSC_TIME)
		{ oss << sep << "^3fpd  "; sep = "^2|"; }
	if(sk_cfg.stats_cols & skivvy_conf::RSC_TIME)
		{ oss << sep << "^3cpd  "; sep = "^2|"; }
	skivvy.chat('s', oss.str());

	for(guid_stat_citer p = stats.begin(); p != stats.end(); ++p)
	{
		const str& player = players.at(p->first);
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

//			siz k = map_get(p->second.kills, MOD_RAILGUN);
//			k += map_get(p->second.kills, MOD_GAUNTLET);
//			siz d = map_get(p->second.deaths, MOD_RAILGUN);
//			d += map_get(p->second.deaths, MOD_GAUNTLET);
		siz h = p->second.logged_time;
		con("c: " << c);
		con("k: " << k);
		con("d: " << d);

		double rkd = 0.0;
		double rcd = 0.0;
		siz rkh = 0;
		siz rch = 0;
		str kd, cd, kh, ch;
		if(!d)
		{
			if(k)
				kd = "perf ";
			if(c)
				cd = "perf  ";
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
			if(sk_cfg.stats_cols & skivvy_conf::RSC_TIME)
			{
				col = "^7" + mins + "^3:^7" + secs;
				set_width(col, 5, 6);
				oss << sep << col;
				sep = "^2|";
			}
			if(sk_cfg.stats_cols & skivvy_conf::RSC_FPH)
			{
				col = "^7" + kh;
				set_width(col, 3, 2);
				oss << sep << col;
				sep = "^2|";
			}
			if(sk_cfg.stats_cols & skivvy_conf::RSC_CPH)
			{
				col = "^7" + ch;
				set_width(col, 3, 2);
				oss << sep << col;
				sep = "^2|";
			}
			if(sk_cfg.stats_cols & skivvy_conf::RSC_KPD)
			{
				col = "^7" + kd;
				set_width(col, 5, 2);
				oss << sep << col;
				sep = "^2|";
			}
			if(sk_cfg.stats_cols & skivvy_conf::RSC_CPD)
			{
				col = "^7" + cd;
				set_width(col, 5, 2);
				oss << sep << col;
				sep = "^2|";
			}

			oss << sep << "^7" << player;
//				oss << "^3time: ^7" << mins << "^3:^7" << secs << " " << "^3kills^7/^3d ^5(^7" << kd << "^5) ^3caps^7/^3d ^5(^7" << cd << "^5)^7: " + player;
			skivvy_scores.insert(std::make_pair(rkh, oss.str()));
		}
	}
	for(std::multimap<double, str>::reverse_iterator r = skivvy_scores.rbegin(); r != skivvy_scores.rend(); ++r)
		skivvy.chat('s', r->second);

	return true;
}

virtual bool KatinaPluginStats::shutdown_game(GameInfo& gi)
{
	in_game = false;
	return true;
}

virtual bool KatinaPluginStats::warmup(GameInfo& gi)
{
	in_game = false;
	return true;
}

virtual bool KatinaPluginStats::client_userinfo_changed(GameInfo& gi, siz num, siz team, const GUID& guid, const str& name)
{
	if(!in_game)
		return true;
	return true;
}
virtual bool KatinaPluginStats::client_connect(GameInfo& gi, siz num)
{

}
virtual bool KatinaPluginStats::client_disconnect(GameInfo& gi, siz num)
{
	if(!in_game)
		return true;
	return true;
}
virtual bool KatinaPluginStats::kill(GameInfo& gi, siz num1, siz num2, siz weap)
{
	if(!in_game)
		return true;

	if(weap != MOD_SUICIDE && gi.clients.find(num1) != gi.clients.end() && gi.clients.find(num2) != gi.clients.end())
		skivvy.chat('k', "^7" + gi.players[gi.clients[num1]] + " ^4killed ^7" + gi.players[gi.clients[num2]]
			+ " ^4with a ^7" + weapons[weap]);

	return true;
}
virtual bool KatinaPluginStats::ctf(GameInfo& gi, siz num, siz team, siz act)
{
	if(!in_game)
		return true;

	siz pcol = team - 1; // make 0-1 for array index
	siz ncol = pcol ? 0 : 1;

	str nums_team = "^7[^2U^7]"; // unknown team
	str nums_nteam = "^7[^2U^7]"; // unknown team

	if(gi.teams[gi.clients[num]] == TEAM_R)
		nums_team = "^7[^1R^7]";
	else if(gi.teams[gi.clients[num]] == TEAM_B)
		nums_team = "^7[^4B^7]";
	if(gi.teams[gi.clients[num]] == TEAM_B)
		nums_nteam = "^7[^1R^7]";
	else if(gi.teams[gi.clients[num]] == TEAM_R)
		nums_nteam = "^7[^4B^7]";

	str hud;

	if(act == FL_CAPTURED) // In Game Announcer
	{
		if(ka_cfg.do_flags)
		{
			str msg = gi.players[gi.clients[num]] + "^3 has ^7" + to_string(caps[gi.clients[num]]) + "^3 flag" + (caps[gi.clients[num]]==1?"":"s") + "!";
			server.cp(msg);
			if(sk_cfg.do_flags)
			{
				if(sk_cfg.do_flags_hud)
				{
					hud_flag[pcol] = HUD_FLAG_CAP;
					hud = get_hud(m, s, hud_flag);
				}
				skivvy.raw_chat('f', hud + oa_to_IRC(nums_team + " " + msg));
				if(sk_cfg.do_flags_hud)
				{
					hud_flag[pcol] = HUD_FLAG_NONE;
					hud = get_hud(m, s, hud_flag);
				}
				skivvy.raw_chat('f', hud + oa_to_IRC("^7[ ] ^1RED^3: ^7" + to_string(flags[FL_BLUE]) + " ^3v ^4BLUE^3: ^7" + to_string(flags[FL_RED])));
			}
		}
	}
	else if(act == FL_TAKEN)
	{
		if(dashing[pcol])
			dash[pcol] = get_millitime();

		if(sk_cfg.do_flags)
		{
			if(sk_cfg.do_flags_hud)
			{
				hud_flag[pcol] = HUD_FLAG_P;
				hud = get_hud(m, s, hud_flag);
			}
			skivvy.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + gi.players[gi.clients[num]] + "^3 has taken the " + flag[pcol] + " ^3flag!"));
		}
	}
	else if(act == FL_DROPPED)
	{
		if(sk_cfg.do_flags)
		{
			if(sk_cfg.do_flags_hud)
			{
				hud_flag[ncol] = HUD_FLAG_DIE;
				hud = get_hud(m, s, hud_flag);
				hud_flag[ncol] = HUD_FLAG_NONE;
			}
			skivvy.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + gi.players[gi.clients[num]] + "^3 has killed " + gi.players[dasher[ncol]] + " the " + flag[ncol] + " ^3flag carrier!"));
		}
		GUID dasher_guid = dasher[ncol];
		dasher[ncol] = null_guid;; // end a dash
		dashing[ncol] = false; // no more dashes until return, capture or suicide
	}
	else if(act == FL_RETURNED)
	{
		dasher[pcol] = null_guid;; // end a dash
		dashing[pcol] = true; // new dash now possible
		if(sk_cfg.do_flags)
		{
			if(sk_cfg.do_flags_hud)
			{
				hud_flag[pcol] = HUD_FLAG_RETURN;
				hud = get_hud(m, s, hud_flag);
				hud_flag[pcol] = HUD_FLAG_NONE;
			}
			skivvy.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + gi.players[gi.clients[num]] + "^3 has returned the " + flag[pcol] + " ^3flag!"));
		}
	}

	return true;
}
virtual bool KatinaPluginStats::award(GameInfo& gi)
{
	if(!in_game)
		return true;

	return true;
}
virtual bool KatinaPluginStats::init_game(GameInfo& gi)
{
	if(in_game)
		return true;

	if(sk_cfg.do_infos && gi.mapname != old_mapname)
	{
		siz love = 0;
		siz hate = 0;
		for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
		{
			if(i->second > 0)
				++love;
			else
				++hate;
		}
		skivvy.chat('i', ".");
		skivvy.chat('i', "^3== Playing Map: ^7" + gi.mapname + "^3 == ^7" + to_string(love)
			+ " ^1LOVE ^7" + to_string(hate) + " ^2HATE ^3==");
		old_mapname = gi.mapname;
	}

	in_game = true;

	return true;
}
virtual bool KatinaPluginStats::say(GameInfo& gi, const str& text)
{
	if(sk_cfg.do_chats)
	{
		str text;
		GUID guid;

		if(extract_name_from_text(line, guid, text))
			if(!sk_cfg.spamkill || ++spam[text] < spam_limit)
				skivvy.chat('c', "^7say: " + gi.players[guid] + " ^2" + text);
	}

	return true;
}
virtual bool KatinaPluginStats::unknown(GameInfo& gi)
{

}

virtual void KatinaPluginStats::close()
{

}

}} // katina::plugin