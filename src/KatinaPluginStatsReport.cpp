
#include "KatinaPlugin.h"
#include "KatinaPluginStats.h"

#include "Database.h"
#include "GUID.h"

#include "types.h"
#include "log.h"

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

virtual bool KatinaPluginStats::open(Katina& katina)
{
	if(!katina.has_plugin("katina::stats", "0.0"))
		return false;

	if(!(kpsp = katina_get_pligin_raw_ptr("katina::stats"))
		return false;

	skivvy.config(recs["skivvy.host"], to<siz>(recs["skivvy.port"]));

	return true;
}

virtual str KatinaPluginStats::get_id() const
{
	return "katina::stats::report";
}

virtual str KatinaPluginStats::get_name() const
{
	return "Stats Reporting";
}

virtual str KatinaPluginStats::get_version() const
{
	return "0.1-dev";
}

virtual bool KatinaPluginStats::exit(GameInfo& gi)
{
	if(!in_game)
		return true;
	in_game = false;

	std::multimap<double, str> skivvy_scores;

	soss oss;
	if(sk_cfg.do_stats)
	{
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
	}
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
		if(sk_cfg.do_stats)
		{
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
	}
	if(sk_cfg.do_stats)
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

	bug("TIMER: joined_time: " << stats[gi.clients[num]].joined_time);

	std::time_t now = std::time(0);

	if(stats[gi.clients[num]].joined_time)
		stats[gi.clients[num]].logged_time += now - stats[gi.clients[num]].joined_time;

	if(gi.teams[gi.clients[num]] == TEAM_R || gi.teams[gi.clients[num]] == TEAM_B)
		stats[gi.clients[num]].joined_time = now;
	else
		stats[gi.clients[num]].joined_time = 0;

	bug("TIMER: logged_time: " << stats[gi.clients[num]].logged_time);
	bug("TIMER: joined_time: " << stats[gi.clients[num]].joined_time);
	bug("TIMER:");

	return true;
}
virtual bool KatinaPluginStats::client_connect(GameInfo& gi, siz num)
{

}
virtual bool KatinaPluginStats::client_disconnect(GameInfo& gi, siz num)
{
	if(!in_game)
		return true;

	std::time_t now = std::time(0);

	if(stats[gi.clients[num]].joined_time)
		stats[gi.clients[num]].logged_time += now - stats[gi.clients[num]].joined_time;
	stats[gi.clients[num]].joined_time = 0;

	return true;
}
virtual bool KatinaPluginStats::kill(GameInfo& gi, siz num1, siz num2, siz weap)
{
	if(!in_game)
		return true;

	if(gi.clients.find(num1) != gi.clients.end() && gi.clients.find(num2) != gi.clients.end())
	{
		if(num1 == 1022 && !gi.clients[num2].is_bot()) // no killer
			++stats[gi.clients[num2]].deaths[weap];
		else if(!gi.clients[num1].is_bot() && !gi.clients[num2].is_bot())
		{
			if(num1 != num2)
			{
				++stats[gi.clients[num1]].kills[weap];
				++onevone[gi.clients[num1]][gi.clients[num2]];
			}
			++stats[gi.clients[num2]].deaths[weap];
		}
	}

	return true;
}
virtual bool KatinaPluginStats::ctf(GameInfo& gi, siz num, siz team, siz act)
{
	if(!in_game)
		return true;

	if(!gi.clients[num].is_bot())
		++stats[gi.clients[num]].flags[act];

	return true;
}
virtual bool KatinaPluginStats::award(GameInfo& gi)
{
	if(!in_game)
		return true;

	++stats[gi.clients[num]].awards[awd];

	return true;
}
virtual bool KatinaPluginStats::init_game(GameInfo& gi)
{
	if(in_game)
		return true;

	stats.clear();
	onevone.clear();

	in_game = true;

	return true;
}
virtual bool KatinaPluginStats::say(GameInfo& gi, const str& text)
{

}
virtual bool KatinaPluginStats::unknown(GameInfo& gi)
{

}

virtual void KatinaPluginStats::close()
{

}

}} // katina::plugin