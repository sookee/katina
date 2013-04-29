
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

siz map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.end() ? 0 : m.at(key);
}

virtual bool KatinaPluginStats::open(str_map& config)
{
	host = config["rcon.host"];
	port = config["rcon.port"];
	db.config(config["db.host"], to<siz>(config["db.port"]), config["db.user"], config["db.pass"], config["db.base"]);
}

virtual str KatinaPluginStats::get_id() const
{
	return "katina::stats";
}

virtual str KatinaPluginStats::get_name() const
{
	return "Stats Collecting";
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

	// in game timing
	for(guid_stat_iter i = stats.begin(); i != stats.end(); ++i)
	{
		bug("TIMER:         EOG: " << i->first);
		if(i->second.joined_time);
		{
			bug("TIMER:         ADD: " << i->first);
			bug("TIMER:         now: " << now);
			bug("TIMER: logged_time: " << i->second.logged_time);
			bug("TIMER: joined_time: " << i->second.joined_time);
			if(i->second.joined_time)
				i->second.logged_time += now - i->second.joined_time;
			i->second.joined_time = 0;
		}
	}

	db.on();

	game_id id = db.add_game(host, port, gi.mapname);

	if(id != null_id && id != bad_id)
	{
		// TODO: insert game stats here
		for(guid_stat_citer p = stats.begin(); p != stats.end(); ++p)
		{
			const str& player = gi.players.at(p->first);

			siz count;
			for(std::set<siz>::iterator weap = ka_cfg.db_weaps.begin(); weap != ka_cfg.db_weaps.end(); ++weap)
			{
				if((count = map_get(p->second.kills, *weap)))
					db.add_weaps(id, "kills", p->first, *weap, count);
				if((count = map_get(p->second.deaths, *weap)))
					db.add_weaps(id, "deaths", p->first, *weap, count);
			}

			if((count = map_get(p->second.flags, FL_CAPTURED)))
				db.add_caps(id, p->first, count);

			if(!p->first.is_bot())
				if((count = p->second.logged_time))
					db.add_time(id, p->first, count);
		}

		for(onevone_citer o = onevone.begin(); o != onevone.end(); ++o)
			for(guid_siz_citer p = o->second.begin(); p != o->second.end(); ++p)
				db.add_ovo(id, o->first, p->first, p->second);
	}

	for(guid_str_map::iterator player = gi.players.begin(); player != gi.players.end(); ++player)
		if(!player->first.is_bot())
			db.add_player(player->first, player->second);

	db.off();

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