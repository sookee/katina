#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_ADMIN_H
#define	_OASTATS_KATINA_PLUGIN_ADMIN_H
/*
 * File:   KatinaPluginAdmin.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 8, 2014
 */

#include <map>
#include <list>
#include <utility>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/Database.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

#include <pthread.h>

namespace katina { namespace plugin {

using namespace oastats;
using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

struct total_ban
{
	str_vec guids;
	str_vec ips;
};

//typedef std::list<total_ban> total_ban_lst;
//typedef total_ban_lst::iterator total_ban_lst_iter;

// guid, type, expires
struct sanction
{
	GUID guid;
	siz type;
	time_t expires;
	str reason;
	str_vec params;

	bool applied; // not stored to file

	sanction(): type(0), expires(0), applied(false) {}

	friend sis& operator>>(sis& is, sanction& s)
	{
		return sgl(is >> s.guid >> s.type >> s.expires >> std::ws, s.reason);
	}

	friend sos& operator<<(sos& os, const sanction& s)
	{
		return os << s.guid << ' ' << s.type << ' ' << s.expires << s.reason;
	}
};

typedef std::list<sanction> sanction_lst;
typedef sanction_lst::iterator sanction_lst_iter;

class KatinaPluginAdmin
: public KatinaPlugin
{
private:
	str& mapname;
	siz_guid_map& clients; // slot -> GUID
	guid_str_map& players; // GUID -> name
	guid_siz_map& teams; // GUID -> 'R' | 'B'
	RCon& server;
	
	bool active;

	sanction_lst sanctions;
	total_ban total_bans;

	// !fixteams - oneshot team shuffle
	// used to calculate kills/caps ratio
	siz total_kills;
	siz total_caps;

	siz_map kills; // num -> kills
	siz_map caps; // num -> capss
	// - /fixteams

	// spamkill
	struct spam
	{
		siz num; // slot
		std::time_t when;
	};

	typedef std::list<spam> spam_lst;
	typedef spam_lst::iterator spam_lst_iter;
	typedef spam_lst::const_iterator spam_lst_citer;
	typedef std::map<str, spam_lst> spam_map;
	typedef std::map<siz, std::time_t> mute_map;

	spam_map spams;
	mute_map mutes;

	siz spamkill_warn; // number of identical msgs before !warn
	siz spamkill_mute; // number of identical msgs before !mute
	std::time_t spamkill_period; // period
	std::time_t spamkill_mute_period; // duration of mute
	// spamkill

	str trans(const str& cmd) const;

	void spamkill(siz num);
	bool fixteams();

	bool mutepp(siz num);
	bool un_mutepp(siz num);
	bool fixname(siz num, const str& name);

	/**
	 * !warn a player next time they connect
	 */
	bool warn_on_sight(siz num, const str& reason);
	bool reteam(siz num, char team = 's');

	bool check_admin(const GUID& guid);
	bool check_slot(siz num);
	bool load_total_bans();
	bool load_sanctions();
	bool save_sanctions();

	/**
	 * Remove all sanctions of a given type.
	 */
	void remove_sanctions(const GUID& guid, siz type);
//	bool apply_sanction(sanction_lst_iter& s);
//	bool apply_sanctions();

public:
	KatinaPluginAdmin(Katina& katina);

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	//virtual void cvar_event(const str& name, const str& value);
	
	virtual bool init_game(siz min, siz sec, const str_map& cvars);
	virtual bool warmup(siz min, siz sec);
	virtual bool client_connect(siz min, siz sec, siz num);
	virtual bool client_connect_info(siz min, siz sec, siz num, const GUID& guid, const str& ip);
	virtual bool client_begin(siz min, siz sec, siz num);
	virtual bool client_disconnect(siz min, siz sec, siz num);
	virtual bool client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name, siz hc);
	virtual bool kill(siz min, siz sec, siz num1, siz num2, siz weap);
	virtual bool award(siz min, siz sec, siz num, siz awd);
	virtual bool ctf(siz min, siz sec, siz num, siz team, siz act);
	virtual bool ctf_exit(siz min, siz sec, siz r, siz b);
	virtual bool score_exit(siz min, siz sec, int score, siz ping, siz num, const str& name);
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
	virtual bool shutdown_game(siz min, siz sec);
	virtual bool exit(siz min, siz sec);
	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params);

	virtual void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_ADMIN_H
