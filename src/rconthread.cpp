/*
 * rconthread.cpp
 *
 *  Created on: Apr 10, 2013
 *      Author: oasookee@gmail.com
 */

#include "rconthread.h"

#include "time.h"
#include "types.h"

#include "Katina.h"
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

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void* rconthread(void* vp)
{
	Katina& kat = *reinterpret_cast<Katina*>(vp);

	if(kat.thread_delay < 3000)
		kat.thread_delay = 3000;

	while(!kat.done)
	{
		thread_sleep_millis(kat.thread_delay);

		// cvar controls

		static siz c = 0;

		server_conf old_ka_cfg = kat.svr_cfg;
		remote_conf old_sk_cfg = kat.rep_cfg;

		str cvar;
		siss iss;
		siz weap;

		switch(c++)
		{
			case 0:
				if(!kat.server.get_cvar("katina_active", kat.svr_cfg.active))
					kat.server.get_cvar("katina_active", kat.svr_cfg.active); // one retry
				if(kat.svr_cfg.active != old_ka_cfg.active)
				{
					log("katina: " + str(kat.svr_cfg.active?"":"de-") + "activated");
					kat.server.chat("^3going ^1" + str(kat.svr_cfg.active?"on":"off") + "-line^3.");
					kat.remote->chat('*', "^3going ^1" + str(kat.svr_cfg.active?"on":"off") + "-line^3.");
				}
			break;
			case 1:
				if(!kat.server.get_cvar("katina_flags", kat.svr_cfg.do_flags))
					kat.server.get_cvar("katina_flags", kat.svr_cfg.do_flags); // one retry
				if(kat.svr_cfg.do_flags != old_ka_cfg.do_flags)
				{
					log("katina: flag counting is now: " << (kat.svr_cfg.do_flags ? "on":"off"));
					kat.server.chat( "^3Flag countng ^1" + str(kat.svr_cfg.do_flags ? "on":"off") + "^3.");
					kat.remote->chat('f', "^3Flag countng ^1" + str(kat.svr_cfg.do_flags ? "on":"off") + "^3.");
				}
			break;
			case 2:
				if(!kat.server.get_cvar("katina_dashes", kat.svr_cfg.do_dashes))
					kat.server.get_cvar("katina_dashes", kat.svr_cfg.do_dashes); // one retry
				if(kat.svr_cfg.do_dashes != old_ka_cfg.do_dashes)
				{
					log("katina: flag timing is now: " << (kat.svr_cfg.do_dashes ? "on":"off"));
					kat.server.chat("^3Flag timing ^1" + str(kat.svr_cfg.do_dashes ? "on":"off") + "^3.");
					kat.remote->chat('f', "^3Flag timing ^1" + str(kat.svr_cfg.do_dashes ? "on":"off") + "^3.");
				}
			break;
			case 3:
				if(!kat.server.get_cvar("katina_db_active", kat.svr_cfg.do_db))
					kat.server.get_cvar("katina_db_active", kat.svr_cfg.do_db); // one retry
				if(kat.svr_cfg.do_db != old_ka_cfg.do_db)
				{
					log("katina: database writing is now: " << (kat.svr_cfg.do_db ? "on":"off"));
					kat.remote->chat('*', "^3Flag timing ^1" + str(kat.svr_cfg.do_db ? "on":"off") + "^3.");
					if(!kat.svr_cfg.do_db)
						kat.db.off();
					else
					{
						kat.db.on();
						kat.db.read_map_votes(kat.mapname, kat.map_votes);
					}
				}
			break;
			case 4:
				if(!kat.server.get_cvar("katina_db_weaps", cvar))
					if(!kat.server.get_cvar("katina_db_weaps", cvar))
						break;

				iss.clear();
				iss.str(cvar);
				while(iss >> weap)
					kat.svr_cfg.db_weaps.insert(weap);

				if(kat.svr_cfg.db_weaps != old_ka_cfg.db_weaps)
				{
					log("katina: database weaps set to: " << cvar);
					kat.remote->chat('*', "^3Database weapons set to: ^1" + cvar + "^3.");
				}
			break;
			case 5:
				if(!kat.server.get_cvar("katina_skivvy_active", kat.rep_cfg.active))
					kat.server.get_cvar("katina_skivvy_active", kat.rep_cfg.active); // one retry
				if(kat.rep_cfg.active != old_sk_cfg.active)
				{
					if(kat.rep_cfg.active)
					{
						log("skivvy: reporting activated");
						kat.remote->chat('*', "^3reporting turned on.");
						kat.remote->on();
					}
					else
					{
						log("skivvy: reporting deactivated");
						kat.remote->chat('*', "^3reporting turned off.");
						kat.remote->off();
					}
				}
			break;
			case 6:
				if(!kat.server.get_cvar("katina_skivvy_chans", kat.rep_cfg.chans))
					kat.server.get_cvar("katina_skivvy_chans", kat.rep_cfg.chans); // one retry
				if(old_sk_cfg.chans != kat.rep_cfg.chans)
				{
					log("skivvy: new chans: " << kat.rep_cfg.chans);
					kat.remote->set_chans(kat.rep_cfg.chans);
					kat.remote->chat('*', "^3Now reporting to ^7" + kat.rep_cfg.chans);
				}
			break;
			case 7:
				if(!kat.server.get_cvar("katina_skivvy_chats", kat.rep_cfg.do_chats))
					kat.server.get_cvar("katina_skivvy_chats", kat.rep_cfg.do_chats); // one retry
				if(kat.rep_cfg.do_chats != old_sk_cfg.do_chats)
				{
					log("skivvy: chat reporting is now: " << (kat.rep_cfg.do_chats ? "on":"off"));
					kat.remote->chat('*', "^3Chat reports ^1" + str(kat.rep_cfg.do_chats ? "on":"off") + "^3.");
				}
			break;
			case 8:
				if(!kat.server.get_cvar("katina_skivvy_flags", kat.rep_cfg.do_flags))
					kat.server.get_cvar("katina_skivvy_flags", kat.rep_cfg.do_flags); // one retry
				if(kat.rep_cfg.do_flags != old_sk_cfg.do_flags)
				{
					log("skivvy: flag reporting is now: " << (kat.rep_cfg.do_flags ? "on":"off"));
					kat.remote->chat('*', "^3Flag reports ^1" + str(kat.rep_cfg.do_flags ? "on":"off") + "^3.");
				}
			break;
			case 9:
				if(!kat.server.get_cvar("katina_skivvy_flags_hud", kat.rep_cfg.do_flags_hud))
					kat.server.get_cvar("katina_skivvy_flags_hud", kat.rep_cfg.do_flags_hud); // one retry
				if(kat.rep_cfg.do_flags_hud != old_sk_cfg.do_flags_hud)
				{
					log("skivvy: flag HUD is now: " << (kat.rep_cfg.do_flags_hud ? "on":"off"));
					kat.remote->chat('*', "^3Flag HUD ^1" + str(kat.rep_cfg.do_flags_hud ? "on":"off") + "^3.");
				}
			break;
			case 10:
				if(!kat.server.get_cvar("katina_skivvy_kills", kat.rep_cfg.do_kills))
					kat.server.get_cvar("katina_skivvy_kills",kat.rep_cfg. do_kills); // one retry
				if(kat.rep_cfg.do_kills != old_sk_cfg.do_kills)
				{
					log("skivvy: kill reporting is now: " << (kat.rep_cfg.do_kills ? "on":"off"));
					kat.remote->chat('*', "^3Kill reports ^1" + str(kat.rep_cfg.do_kills ? "on":"off") + "^3.");
				}
			break;
			case 11:
				if(!kat.server.get_cvar("katina_skivvy_infos", kat.rep_cfg.do_infos))
					kat.server.get_cvar("katina_skivvy_infos", kat.rep_cfg.do_infos); // one retry
				if(kat.rep_cfg.do_kills != old_sk_cfg.do_kills)
				{
					log("skivvy: info reporting is now: " << (kat.rep_cfg.do_infos ? "on":"off"));
					kat.remote->chat('*', "^3Info reports ^1" + str(kat.rep_cfg.do_infos ? "on":"off") + "^3.");
				}
			break;
			case 12:
				if(!kat.server.get_cvar("katina_skivvy_stats", kat.rep_cfg.do_stats))
					kat.server.get_cvar("katina_skivvy_stats", kat.rep_cfg.do_stats); // one retry
				if(kat.rep_cfg.do_stats != old_sk_cfg.do_stats)
				{
					log("skivvy: stats reporting is now: " << (kat.rep_cfg.do_stats ? "on":"off"));
					kat.remote->chat('*', "^3Stats reports ^1" + str(kat.rep_cfg.do_stats ? "on":"off") + "^3.");
				}
			break;
			case 13:
				if(!kat.server.get_cvar("katina_skivvy_spamkill", kat.rep_cfg.spamkill))
					kat.server.get_cvar("katina_skivvy_spamkill", kat.rep_cfg.spamkill); // one retry
				if(old_sk_cfg.spamkill != kat.rep_cfg.spamkill)
				{
					log("skivvy: spamkill is now: " << (kat.rep_cfg.spamkill ? "on":"off"));
					kat.remote->chat('*', "^3Spamkill ^1" + str(kat.rep_cfg.spamkill ? "on":"off") + "^3.");
				}
			break;
			default:
				c = 0;
			break;
		}

		if(!kat.rep_cfg.active || !kat.rep_cfg.active)
			continue;

		str reply;
		if(kat.server.command("!listplayers", reply))
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
						kat.teams[kat.clients[n]] = team;
						pthread_mutex_unlock(&mtx);
					}
				}
			}
		}
	}
	pthread_exit(0);
}

} // oastats
