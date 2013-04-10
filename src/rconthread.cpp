/*
 * rconthread.cpp
 *
 *  Created on: Apr 10, 2013
 *      Author: oasookee@gmail.com
 */

#include "rconthread.h"

#include "time.h"
#include "types.h"

#include "Database.h"
#include "RemoteIRCClient.h"
#include "rcon.h"
#include "str.h"

#include <pthread.h>

namespace oastats
{

using namespace oastats::net;
using namespace oastats::time;
using namespace oastats::types;
using namespace oastats::string;

void* rconthread(void* td_vp)
{
	thread_data& td = *reinterpret_cast<thread_data*>(td_vp);

	pthread_mutex_t& mtx = *td.mtx_p;
	siz_guid_map& clients = *td.clients_p;
	guid_siz_map& teams = *td.teams_p;
	bool& done = *td.done_p;
	server_conf& svr_cfg = *td.svr_cfg_p;
	remote_conf& rep_cfg = *td.rep_cfg_p;
	RCon& server = *td.server_p;
	RemoteIRCClient* remote = td.remote_p;
	Database& db = *td.db_p;
	str& mapname = *td.mapname_p;
	guid_int_map& map_votes = *td.map_votes_p;

	if(td.delay < 3000)
		td.delay = 3000;

	while(!done)
	{
		thread_sleep_millis(td.delay);

		// cvar controls

		static siz c = 0;

		server_conf old_ka_cfg = svr_cfg;
		remote_conf old_sk_cfg = rep_cfg;
		str cvar;
		siss iss;
		siz weap;

		switch(c++)
		{
			case 0:
				if(!server.get_cvar("katina_active", svr_cfg.active))
					server.get_cvar("katina_active", svr_cfg.active); // one retry
				if(svr_cfg.active != old_ka_cfg.active)
				{
					log("katina: " + str(svr_cfg.active?"":"de-") + "activated");
					server.chat("^3going ^1" + str(svr_cfg.active?"on":"off") + "-line^3.");
					remote->chat('*', "^3going ^1" + str(svr_cfg.active?"on":"off") + "-line^3.");
				}
			break;
			case 1:
				if(!server.get_cvar("katina_flags", svr_cfg.do_flags))
					server.get_cvar("katina_flags", svr_cfg.do_flags); // one retry
				if(svr_cfg.do_flags != old_ka_cfg.do_flags)
				{
					log("katina: flag counting is now: " << (svr_cfg.do_flags ? "on":"off"));
					server.chat( "^3Flag countng ^1" + str(svr_cfg.do_flags ? "on":"off") + "^3.");
					remote->chat('f', "^3Flag countng ^1" + str(svr_cfg.do_flags ? "on":"off") + "^3.");
				}
			break;
			case 2:
				if(!server.get_cvar("katina_dashes", svr_cfg.do_dashes))
					server.get_cvar("katina_dashes", svr_cfg.do_dashes); // one retry
				if(svr_cfg.do_dashes != old_ka_cfg.do_dashes)
				{
					log("katina: flag timing is now: " << (svr_cfg.do_dashes ? "on":"off"));
					server.chat("^3Flag timing ^1" + str(svr_cfg.do_dashes ? "on":"off") + "^3.");
					remote->chat('f', "^3Flag timing ^1" + str(svr_cfg.do_dashes ? "on":"off") + "^3.");
				}
			break;
			case 3:
				if(!server.get_cvar("katina_db_active", svr_cfg.do_db))
					server.get_cvar("katina_db_active", svr_cfg.do_db); // one retry
				if(svr_cfg.do_db != old_ka_cfg.do_db)
				{
					log("katina: database writing is now: " << (svr_cfg.do_db ? "on":"off"));
					remote->chat('*', "^3Flag timing ^1" + str(svr_cfg.do_db ? "on":"off") + "^3.");
					if(!svr_cfg.do_db)
						db.off();
					else
					{
						db.on();
						db.read_map_votes(mapname, map_votes);
					}
				}
			break;
			case 4:
				if(!server.get_cvar("katina_db_weaps", cvar))
					if(!server.get_cvar("katina_db_weaps", cvar))
						break;

				iss.clear();
				iss.str(cvar);
				while(iss >> weap)
					svr_cfg.db_weaps.insert(weap);

				if(svr_cfg.db_weaps != old_ka_cfg.db_weaps)
				{
					log("katina: database weaps set to: " << cvar);
					remote->chat('*', "^3Database weapons set to: ^1" + cvar + "^3.");
				}
			break;
			case 5:
				if(!server.get_cvar("katina_skivvy_active", rep_cfg.active))
					server.get_cvar("katina_skivvy_active", rep_cfg.active); // one retry
				if(rep_cfg.active != old_sk_cfg.active)
				{
					if(rep_cfg.active)
					{
						log("skivvy: reporting activated");
						remote->chat('*', "^3reporting turned on.");
						remote->on();
					}
					else
					{
						log("skivvy: reporting deactivated");
						remote->chat('*', "^3reporting turned off.");
						remote->off();
					}
				}
			break;
			case 6:
				if(!server.get_cvar("katina_skivvy_chans", rep_cfg.chans))
					server.get_cvar("katina_skivvy_chans", rep_cfg.chans); // one retry
				if(old_sk_cfg.chans != rep_cfg.chans)
				{
					log("skivvy: new chans: " << rep_cfg.chans);
					remote->set_chans(rep_cfg.chans);
					remote->chat('*', "^3Now reporting to ^7" + rep_cfg.chans);
				}
			break;
			case 7:
				if(!server.get_cvar("katina_skivvy_chats", rep_cfg.do_chats))
					server.get_cvar("katina_skivvy_chats", rep_cfg.do_chats); // one retry
				if(rep_cfg.do_chats != old_sk_cfg.do_chats)
				{
					log("skivvy: chat reporting is now: " << (rep_cfg.do_chats ? "on":"off"));
					remote->chat('*', "^3Chat reports ^1" + str(rep_cfg.do_chats ? "on":"off") + "^3.");
				}
			break;
			case 8:
				if(!server.get_cvar("katina_skivvy_flags", rep_cfg.do_flags))
					server.get_cvar("katina_skivvy_flags", rep_cfg.do_flags); // one retry
				if(rep_cfg.do_flags != old_sk_cfg.do_flags)
				{
					log("skivvy: flag reporting is now: " << (rep_cfg.do_flags ? "on":"off"));
					remote->chat('*', "^3Flag reports ^1" + str(rep_cfg.do_flags ? "on":"off") + "^3.");
				}
			break;
			case 9:
				if(!server.get_cvar("katina_skivvy_flags_hud", rep_cfg.do_flags_hud))
					server.get_cvar("katina_skivvy_flags_hud", rep_cfg.do_flags_hud); // one retry
				if(rep_cfg.do_flags_hud != old_sk_cfg.do_flags_hud)
				{
					log("skivvy: flag HUD is now: " << (rep_cfg.do_flags_hud ? "on":"off"));
					remote->chat('*', "^3Flag HUD ^1" + str(rep_cfg.do_flags_hud ? "on":"off") + "^3.");
				}
			break;
			case 10:
				if(!server.get_cvar("katina_skivvy_kills", rep_cfg.do_kills))
					server.get_cvar("katina_skivvy_kills",rep_cfg. do_kills); // one retry
				if(rep_cfg.do_kills != old_sk_cfg.do_kills)
				{
					log("skivvy: kill reporting is now: " << (rep_cfg.do_kills ? "on":"off"));
					remote->chat('*', "^3Kill reports ^1" + str(rep_cfg.do_kills ? "on":"off") + "^3.");
				}
			break;
			case 11:
				if(!server.get_cvar("katina_skivvy_infos", rep_cfg.do_infos))
					server.get_cvar("katina_skivvy_infos", rep_cfg.do_infos); // one retry
				if(rep_cfg.do_kills != old_sk_cfg.do_kills)
				{
					log("skivvy: info reporting is now: " << (rep_cfg.do_infos ? "on":"off"));
					remote->chat('*', "^3Info reports ^1" + str(rep_cfg.do_infos ? "on":"off") + "^3.");
				}
			break;
			case 12:
				if(!server.get_cvar("katina_skivvy_stats", rep_cfg.do_stats))
					server.get_cvar("katina_skivvy_stats", rep_cfg.do_stats); // one retry
				if(rep_cfg.do_stats != old_sk_cfg.do_stats)
				{
					log("skivvy: stats reporting is now: " << (rep_cfg.do_stats ? "on":"off"));
					remote->chat('*', "^3Stats reports ^1" + str(rep_cfg.do_stats ? "on":"off") + "^3.");
				}
			break;
			case 13:
				if(!server.get_cvar("katina_skivvy_spamkill", rep_cfg.spamkill))
					server.get_cvar("katina_skivvy_spamkill", rep_cfg.spamkill); // one retry
				if(old_sk_cfg.spamkill != rep_cfg.spamkill)
				{
					log("skivvy: spamkill is now: " << (rep_cfg.spamkill ? "on":"off"));
					remote->chat('*', "^3Spamkill ^1" + str(rep_cfg.spamkill ? "on":"off") + "^3.");
				}
			break;
			default:
				c = 0;
			break;
		}

		if(!rep_cfg.active || !rep_cfg.active)
			continue;

		str reply;
		if(server.command("!listplayers", reply))
		{
			trim(reply);
			// !listplayers: 4 players connected:
			//  1 R 0   Unknown Player (*)   Major
			//  2 B 0   Unknown Player (*)   Tony
			//  4 B 0   Unknown Player (*)   Sorceress
			//  5 R 0   Unknown Player (*)   Sergei
			if(!reply.empty())
			{
				siz n;
				char team;
				siss iss(reply);
				str line;
				std::getline(iss, line); // skip command
				while(std::getline(iss, line))
				{
					//bug("\t\tline: " << line);
					siss iss(line);
					if(iss >> n >> team)
					{
						pthread_mutex_lock(&mtx);
						teams[clients[n]] = team;
						pthread_mutex_unlock(&mtx);
					}
				}
			}
		}
	}
	pthread_exit(0);
}

} // oastats
