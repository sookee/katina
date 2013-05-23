/*
 * File:   KatinaPluginReports.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 10:02 AM
 */

#ifndef KATINA_PLUGIN_REPORTS_H
#define	KATINA_PLUGIN_REPORTS_H

#include <map>
#include <utility>

#include "KatinaPluginStats.h"
#include "KatinaPluginVotes.h"

#include <katina/Database.h>
#include <katina/RemoteClient.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

#include <katina/PKI.h>

namespace katina { namespace plugin {

using namespace oastats::pki;
using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

class RemoteClientList
//: public RemoteClient
{
	std::vector<RemoteClient*> clients;
	
public:
	RemoteClientList(Katina& katina);
	~RemoteClientList();

	void on() { for(siz i = 0; i < clients.size(); ++i) clients[i]->on(); }
	void off() { for(siz i = 0; i < clients.size(); ++i) clients[i]->off(); }
	
	void add(RemoteClient* client) { if(client) { clients.push_back(client); } }
	void clear()
	{
		for(siz i = 0; i < clients.size(); ++i)
		{
			clients[i]->off();
			delete clients[i];
		}
		clients.clear();
	}
	
	bool chat(char f, const str& text) { for(siz i = 0; i < clients.size(); ++i) clients[i]->chat(f, text); return true; }
	bool raw_chat(char f, const str& text) { for(siz i = 0; i < clients.size(); ++i) clients[i]->raw_chat(f, text); return true; }
	
	bool send(const str& cmd, str& res);
};

class KatinaPluginReports
: public KatinaPlugin
{
public:

private:
	KatinaPluginStats* stats;
	KatinaPluginVotes* votes;
	
	RemoteClientList client;
	
	// %time %fph %cph %kpd %cpd %acc(RG|RL|LG)

	enum
	{
		RSC_TIME     = 0b00000001
		, RSC_FPH    = 0b00000010 // frags/hour
		, RSC_CPH    = 0b00000100 // flags/hour
		, RSC_KPD    = 0b00001000 // kills/deaths
		, RSC_CPD    = 0b00010000 // caps/deaths
		, RSC_RGACC  = 0b00100000 // railgun accuracy
	};

	// cvars
	bool active;
	bool do_flags;
	bool do_flags_hud;
	bool do_chats;
	bool do_kills;
	bool do_infos;
	bool do_stats;
	str stats_cols;
	str stats_sort; // sort column
	bool spamkill;
	str chans;

	str_siz_map spam;
	siz spam_limit;

	siz flags[2];
	guid_siz_map caps; // GUID -> <count>

	str old_mapname;

	str_vec notspam; // spam exceptions
/*
	str get_stats_cols() const
	{
		str cols, sep;
		if(stats_cols & RSC_TIME)
			{ cols += sep + "TIME"; sep = " "; }
		if(stats_cols & RSC_FPH)
			{ cols += sep + "FPH"; sep = " "; }
		if(stats_cols & RSC_CPH)
			{ cols += sep + "CPH"; sep = " "; }
		if(stats_cols & RSC_KPD)
			{ cols += sep + "KPD"; sep = " "; }
		if(stats_cols & RSC_CPD)
			{ cols += sep + "CPD"; sep = " "; }
		return cols;
	}
*/
	str get_nums_team(siz num);
	str get_nums_team(const GUID& guid);

public:
	KatinaPluginReports(Katina& katina);

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool init_game(siz min, siz sec, const str_map& cvars);
	//virtual bool warmup(siz min, siz sec);
	//virtual bool client_connect(siz min, siz sec, siz num);
	//virtual bool client_disconnect(siz min, siz sec, siz num);
	//virtual bool client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name);
	virtual bool kill(siz min, siz sec, siz num1, siz num2, siz weap);
	virtual bool ctf(siz min, siz sec, siz num, siz team, siz act);
	//virtual bool award(siz min, siz sec, siz num, siz awd);
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
	//virtual bool shutdown_game(siz min, siz sec);
	virtual bool exit(siz min, siz sec);
//	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params);

	virtual void close();
};

}} // katina::plugin

#endif	// KATINA_PLUGIN_REPORTS_H

