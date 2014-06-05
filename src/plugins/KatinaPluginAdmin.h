#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_ADMIN_H
#define	_OASTATS_KATINA_PLUGIN_ADMIN_H
/*
 * File:   KatinaPluginAdmin.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 8, 2014
 */

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
#include <map>
#include <list>
#include <utility>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/Database.h>
#include <katina/GUID.h>
#include <katina/RemoteClient.h>

#include <katina/types.h>
#include <katina/log.h>

#include <katina/PKI.h>

#include <pthread.h>

namespace katina { namespace plugin {

using namespace oastats;
using namespace oastats::log;
using namespace oastats::pki;
using namespace oastats::data;
using namespace oastats::types;

struct total_ban
{
	str_vec guids;
	str_vec ips;
};

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

enum class policy_t : byte
{
	FT_NONE
	, FT_EVEN_SCATTER // best to team a, next best to team b etc...
	, FT_EVEN_SCATTER_DB // even scatter using mysql db info`
	, FT_NEAREST_DIFFERENCE // add up teams a and b then switch 1 player to even them
	, FT_BEST_PERMUTATION
	, FT_MAX
};

sos& operator<<(sos& o, const policy_t& p) { return o << static_cast<siz>(p); }
sis& operator>>(sis& i, policy_t& p)
{
	siz b;

	if(i >> b && b >= static_cast<siz>(policy_t::FT_MAX))
		i.setstate(std::ios::failbit);

	if(i)
		p = static_cast<policy_t>(b);

	return i;
}

class KatinaPluginAdmin
: public KatinaPlugin
{
private:
	KatinaPlugin* stats = nullptr;
	RemoteClient* irc = nullptr;

	const str& mapname;
	const slot_guid_map& clients; // slot -> GUID
	const guid_str_map& players; // GUID -> name
	const guid_siz_map& teams; // GUID -> 'R' | 'B'
	RCon& server;
	
	bool active;

	sanction_lst sanctions;
//	total_ban total_bans;

	// !fixteams - oneshot team shuffle
	// used to calculate kills/caps ratio
	siz total_kills;
	siz total_caps;

	slot_siz_map kills; // num -> kills
	slot_siz_map caps; // num -> caps
	slot_siz_map secs; // num -> time in seconds
	slot_siz_map time; // num -> time in seconds

	policy_t policy;

	// - /fixteams

	bool protect_admins = false;

	// spamkill
	struct spam
	{
		slot num; // slot
		std::time_t when;
	};

	typedef std::vector<std::pair<str,str>> str_pair_vec;
	typedef str_pair_vec::iterator str_pair_vec_iter;
	typedef str_pair_vec::const_iterator str_pair_vec_citer;

	typedef std::list<spam> spam_lst;
	typedef spam_lst::iterator spam_lst_iter;
	typedef spam_lst::const_iterator spam_lst_citer;
	typedef std::map<str, spam_lst> spam_map;
	typedef std::map<slot, std::time_t> mute_map;

	spam_map spams;
	mute_map mutes;

	//siz spamkill_warn; // number of identical msgs before !warn

	// allows spamkill_spams / spamkill_period
	// otherwise !mute for spamkill_mute_period

	bool do_spamkill = false;
	siz spamkill_spams; // number of identical msgs / period before !mute
	siz spamkill_period; // period (seconds)
	siz spamkill_mute; // duration of mute (seconds)
	// spamkill

	// votekill
	//str_pair_vec votekills;
//	guid_str_map votebans; // people banned from voting GUID -> "reason"
	// ~votekill

	str trans(const str& cmd) const;

	void tell_perp(slot admin_num, slot perp_num, const str& msg);

	void spamkill(slot num);
	bool fixteams();

	bool mutepp(slot num);
	bool votekill(const str& reason);
	bool un_mutepp(slot num);
	bool fixname(slot num, const str& name);

	/**
	 * !warn a player next time they connect
	 */
	bool warn_on_sight(slot num, const str& reason);
	bool reteam(slot num, char team = 's');

	bool check_admin(const GUID& guid);
	bool check_slot(slot num);
//	bool load_total_bans();
	bool load_sanctions();
	bool save_sanctions();

	/**
	 * Remove all sanctions of a given type.
	 * @return true on success
	 */
	bool remove_sanctions(const GUID& guid, siz type);
//	bool apply_sanction(sanction_lst_iter& s);
//	bool apply_sanctions();

public:
	KatinaPluginAdmin(Katina& katina);

	// INTERFACE: KatinaPlugin

	virtual bool open() override;

	virtual str get_id() const override;
	virtual str get_name() const override;
	virtual str get_version() const override;

	//virtual void cvar_event(const str& name, const str& value);
	
	virtual bool init_game(siz min, siz sec, const str_map& cvars) override;
	virtual bool warmup(siz min, siz sec) override;
	virtual bool client_connect(siz min, siz sec, slot num) override;
	virtual bool client_connect_info(siz min, siz sec, slot num, const GUID& guid, const str& ip) override;
	virtual bool client_begin(siz min, siz sec, slot num) override;
	virtual bool client_disconnect(siz min, siz sec, slot num) override;
	virtual bool client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name, siz hc) override;
	virtual bool client_switch_team(siz min, siz sec, slot num, siz teamBefore, siz teamNow) override;
	virtual bool callvote(siz min, siz sec, slot num, const str& type, const str& info) override;
	virtual bool kill(siz min, siz sec, slot num1, slot num2, siz weap) override;
	virtual bool award(siz min, siz sec, slot num, siz awd);
	virtual bool ctf(siz min, siz sec, slot num, siz team, siz act) override;
	virtual bool ctf_exit(siz min, siz sec, siz r, siz b) override;
	virtual bool score_exit(siz min, siz sec, int score, siz ping, slot num, const str& name) override;
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text) override;
	virtual bool chat(siz min, siz sec, const str& text) override;
	virtual bool shutdown_game(siz min, siz sec) override;
	virtual bool exit(siz min, siz sec) override;
	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params) override;
	virtual void heartbeat(siz min, siz sec) override;
	virtual void close() override;
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_ADMIN_H

