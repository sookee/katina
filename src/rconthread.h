#pragma once
#ifndef _OASTATS_RCONTHREAD_H_
#define _OASTATS_RCONTHREAD_H_
/*
 * rconthread.h
 *
 *  Created on: Apr 10, 2013
 *      Author: oasookee@gmail.com
 */

#include "types.h"
#include "GUID.h"
#include "Database.h"
#include "RemoteIRCClient.h"
#include "rcon.h"

namespace oastats
{

using namespace oastats::net;
using namespace oastats::data;
using namespace oastats::types;

//struct thread_data
//{
//	pthread_mutex_t* mtx_p;
//	milliseconds delay;
//	siz_guid_map* clients_p;
//	guid_siz_map* teams_p;
//	bool* done_p;
//	server_conf* svr_cfg_p;
//	remote_conf* rep_cfg_p;
//	RCon* server_p;
//	RemoteIRCClient* remote_p;
//	Database* db_p;
//	str* mapname_p;
//	guid_int_map* map_votes_p;
//};

extern pthread_mutex_t mtx;

void* rconthread(void* td_vp);

} // oastats

#endif /* _OASTATS_RCONTHREAD_H_ */
