/*
 * File:   Katina.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 1, 2013, 6:23 PM
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

#include <dlfcn.h>
#include <cassert>
#include <ctime>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/rcon.h>

#include <katina/time.h>
#include <katina/types.h>
#include <katina/utils.h>
#include <katina/str.h>
#include <katina/GUID.h>
#include <katina/codes.h>

#include <katina/log.h>

#ifndef REVISION
#define REVISION unset
#endif

#define ID "katina"

//#define QUOTE(x) #x
#define REV QUOTE(REVISION)

namespace oastats {

using namespace oastats::log;
using namespace oastats::net;
using namespace oastats::time;
using namespace oastats::types;
using namespace oastats::utils;
using namespace oastats::string;

const str version = "0.1";
const str tag = "";
const str revision = REV;

const slot bad_slot(-1);

siz Katina::getTeam(slot client) const
{
    slot_guid_map_citer clientsIt = clients.find(client);
    if(clientsIt == clients.end())
        return TEAM_U;
    
    guid_siz_map_citer teamsIt = teams.find(clientsIt->second);
    if(teamsIt == teams.end())
        return TEAM_U;
    
    return teamsIt->second;
}

siz Katina::getTeam(const GUID& guid) const
{
     guid_siz_map_citer teamsIt = teams.find(guid);
    if(teamsIt == teams.end())
        return TEAM_U;

    return teamsIt->second;
}

str Katina::getPlayerName(slot num) const
{
	slot_guid_map_citer c;
	if((c = clients.find(num)) == clients.end())
		return "<unknown>";

	guid_str_map_citer p;
	if((p = players.find(c->second)) == players.cend())
		return "<unknown>";

	return p->second;
}

str Katina::getPlayerName(const GUID& guid) const
{
	guid_str_map_citer p;
	if((p = players.find(guid)) == players.cend())
		return "<unknown>";

	return p->second;
}

slot Katina::getClientSlot(const GUID& guid) const
{
	//bug_func();
	for(slot_guid_map_citer it = clients.cbegin(); it != clients.cend(); ++it)
		if(it->second == guid)
			return it->first;
    return bad_slot;
}

const GUID&  Katina::getClientGuid(slot num) const
{
	for(slot_guid_map_citer it = clients.cbegin(); it != clients.cend(); ++it)
		if(it->first == num)
			return it->second;
    return null_guid;
}

str Katina::get_version() const { return version + (tag.size()?"-":"") + tag; }

Katina::Katina()
: done(false)
, active(true) // TODO: make this false
, logmode(LOG_NORMAL)
, now(std::time(0))
{
}

Katina::~Katina()
{
	done = true;
}

bool Katina::rconset(const str& cvar, str& val)
{
	str response;
	if(!server.command(cvar, response))
	{
		log("WARN: rconset failure: " << cvar);
		return false;
	}

	// Possible responses:
	// -> unknown command: <var>
	// -> "<var>" is:"<val>^7", the default
	// -> "katina_skivvy_chans" is:"#katina-test(c) #katina(c)^7" default:"#katina-test(c)^7"

	trim(response);

	if(response.empty())
		return false;

	if(!response.find("unknown command:"))
		return false;

	str sval, skip;
	siss iss(response);
	if(!std::getline(std::getline(iss, skip, ':').ignore(), sval, '^'))
	{
		log("ERROR: parsing rconset response: " << response);
		return false;
	}

	val = sval;
	return true;
}

GUID Katina::guid_from_name(const str& name)
{
	for(guid_str_map_iter i = players.begin(); i != players.end(); ++i)
		if(i->second == name)
			return i->first;
	return null_guid;
}

bool Katina::extract_name_from_text(const str& line, GUID& guid, str& text)
{
	GUID candidate;
	siz pos = 0;
	siz beg = 0;
	if((beg = line.find(": ")) == str::npos) // "say: "
		return false;

	beg += 2;

	bool found = false;
	for(pos = beg; (pos = line.find(": ", pos)) != str::npos; pos += 2)
	{
		if((candidate = guid_from_name(line.substr(beg, pos - beg))) == null_guid)
			continue;
		guid = candidate;
		text = line.substr(pos + 2);
		found = true;
	}
	return found;
}

bool Katina::load_plugin(const str& file)
{
	void* dl = 0;
	KatinaPlugin* plugin = 0;
	KatinaPlugin* (*katina_plugin_factory)(Katina&) = 0;

	log("PLUGIN LOAD: " << file);

	if(!(dl = dlopen(file.c_str(), RTLD_LAZY|RTLD_GLOBAL)))
	{
		log("PLUGIN LOAD ERROR: dlopen: " << dlerror());
		return false;
	}

	if(!(*(void**)&katina_plugin_factory = dlsym(dl, "katina_plugin_factory")))
	{
		log("PLUGIN LOAD ERROR: dlsym: " << dlerror());
		return false;
	}

	if(!(plugin = katina_plugin_factory(*this)))
	{
		log("PLUGIN LOAD: plugin factory failed");
		if(dlclose(dl))
			log("PLUGIN LOAD: plugin failed to unload: " << dlerror());
		return false;
	}

	plugin->dl = dl;

	if(!plugin->open())
	{
		log("PLUGIN LOAD: plugin failed to open");
		delete plugin;
		if(dlclose(dl))
			log("PLUGIN LOAD: plugin failed to unload: " << dlerror());
		return false;
	}

	plugins[plugin->get_id()] = plugin;
	plugin_files[plugin->get_id()] = file;

	log("PLUGIN LOAD: OK: " << plugin->get_id() << ", " << plugin->get_name() << ", " << plugin->get_version());

	return true;
}

bool Katina::unload_plugin(const str& id)
{
	plugin_map_iter i = plugins.find(id);

	if(i == plugins.end())
	{
		log("PLUGIN UNLOAD: plugin not found: " << id);
		return false;
	}

	i->second->close();
	void* dl = i->second->dl;
	delete i->second;
	plugins.erase(i);

	if(dlclose(dl))
	{
		log("PLUGIN UNLOAD: " << dlerror());
		return false;
	}

	return true;
}

bool Katina::reload_plugin(const str& id)
{
	plugin_map_iter i = plugins.find(id);

	if(i == plugins.end())
	{
		log("PLUGIN RELOAD: ERROR: plugin not found: " << id);
		return false;
	}

	str_map_iter f = plugin_files.find(id);

	if(f == plugin_files.end())
	{
		log("PLUGIN RELOAD: ERROR: plugin file not known: " << id);
		return false;
	}

	if(!unload_plugin(id))
		log("PLUGIN RELOAD: ERROR: plugin '" << id << "' failed to unload: " << f->second);

	if(!load_plugin(f->second))
	{
		log("PLUGIN RELOAD: ERROR: plugin '" << id << "' failed to reload: " << f->second);
		return false;
	}

	return true;
}

KatinaPlugin* Katina::get_plugin(const str& id, const str& version)
{
	plugin_map_iter i = plugins.find(id);

	if(i == plugins.end())
	{
		log("ERROR: plugin not found: " << id);
		return 0;
	}

	if(i->second->get_version() < version)
	{
		log("ERROR: wrong version found: " << i->second->get_version() << " expected " << version);
		return 0;
	}

	return i->second;
}

bool Katina::chat_to(slot num, const str& text)
{
	return chat_to(clients[num], text);
}

bool Katina::chat_to(const GUID& guid, const str& text)
{
	return chat_to(players[guid], text);
}

bool Katina::chat_to(const str& name, const str& text)
{
	return server.s_chat(name + "^2 " + text);
}

bool Katina::is_admin(const GUID& guid)
{
	str_vec admins = get_vec("admin.guid");
	for(str_vec_iter i = admins.begin(); i != admins.end(); ++i)
		if(guid == *i)
			return true;

	// now try admin.dat file
	str admin_dat = get("admin.dat.file");
	if(admin_dat.empty())
		return false;
	sifs ifs(expand_env(admin_dat).c_str());
	if(!ifs)
	{
		log("WARN: admin.dat file not found: " << admin_dat);
		return false;
	}

	str line;
	// [admin]
	// name    = ^1S^2oo^3K^5ee
	// guid    = 87597A67B5A4E3C79544A72B7B5DA741
	while(sgl(ifs, line))
		if(trim(line) == "[admin]")
			if(sgl(sgl(ifs, line), line))
				if(trim(line).size() == 32 && guid == line.substr(0, 8))
					return true;
	return false;
}

bool Katina::load_config(const str& dir, const str& file, property_map& props)
{
	str config_file = config_dir + "/" + expand_env(file);

	log("CONFIG LOAD: " << config_file);

	std::ifstream ifs(config_file.c_str());

	if(!ifs.is_open())
	{
		log("ERROR: opening config file");
		return false;
	}

	// working variables
	siz pos;
	str line, key, val;

	// read in config

	siz no = 0;
	while(sgl(ifs, line))
	{
		++no;

		if((pos = line.find("//")) != str::npos)
			line.erase(pos);

		trim(line);

		if(line.empty() || line[0] == '#')
			continue;

		siss iss(line);
		if(!sgl(sgl(iss, key, ':') >> std::ws, val))
		{
			log("ERROR: parsing config file: " << file << " at: " << no);
			log("ERROR:                    : " << line);
			continue;
		}

		if(logmode > LOG_NORMAL)
			log("CONF: " << key << ": " << val);

		if(key == "include")
			load_config(dir, val, props);
		else if(key == "logmode")
			logmode =  to<int>(val);
		else
//			props[key].push_back(expand_env(val, WRDE_SHOWERR|WRDE_UNDEF));
			props[key].push_back(val);

	}
	ifs.close();
	log("CONFIG LOAD: OK:");
	return true;
}

bool Katina::init_pki()
{
	log("Reading keypair:");

	str keypair_file = get_exp("pki.keypair", "keypair.pki");
//	str pkeys_file = get("pki.pkeys", "pkeys.pki");

	if(!pki.load_keypair(config_dir + "/" + keypair_file))
	{
		log("WARNING: Unable to load keypair: " << keypair_file);
		log("       : Creating a new one");

		if(!pki.generate_keypair(128))
		{
			log("FATAL: Unable to generate keypair");
			return false;
		}

		str sexp;
		if(pki.get_keypair_as_text(sexp))
		{
			std::ofstream ofs((config_dir + "/" + keypair_file).c_str());
			ofs << sexp;
		}

//		if(pki.get_public_key_as_text(sexp))
//		{
//			std::ofstream ofs((config_dir + "/" + pkeys_file).c_str());
//			ofs << sexp;
//		}
	}

	log("Reading public keys:");
	str_vec pkeys = get_exp_vec("pki.pkey");
	for(siz i = 0; i < pkeys.size(); ++i)
	{
		log("LOAD PUBLIC KEY: " << pkeys[i]);
		pki.load_public_key(pkeys[i], pkeys[i]);
	}
    
	// TODO: ad this
	//ifs.open((config_dir + "/" + pkeys_file).c_str());
	//while(pki.read_public_key(, ifs)) {}
	//ifs.close();
    
    return true;
}

void Katina::init_rcon()
{
	log("Initializing rcon:");
	server.config(get("rcon.host"), get<siz>("rcon.port"), get("rcon.pass"));

	if(get("rcon.active") == "true")
		server.on();
    
	prefix = get("rcon.cvar.prefix");
	if(!prefix.empty())
		prefix += ".";
    
	//pthread_create(&cvarevts_thread, 0, &cvarpoll, (void*) this);
}

void Katina::load_plugins()
{
	log("Loading plugins:");
	str_vec pluginfiles = get_exp_vec("plugin");
	for(siz i = 0; i < pluginfiles.size(); ++i)
		load_plugin(pluginfiles[i]);
}

void Katina::builtin_command(const GUID& guid, const str& text)
{
	bug_func();
	bug_var(guid);
	bug_var(text);

	slot num;

	if((num = getClientSlot(guid)) == bad_slot)
	{
		server.s_chat("ERROR: Cannot locate client number.");
		return;
	}

	if(trim_copy(text).empty())
		server.msg_to(num, "^1K^7at^3i^7na: " + revision);

	str cmd;
	siss iss(text);

	static str plugin = "";

	if(!(iss >> cmd >> cmd))
		log("Error: parsing builtin command: " << text);
	else
	{
		bug_var(cmd);
		if(cmd == "plugin")
		{
			if(iss >> plugin)
				server.msg_to(num, "plugin is now: " + plugin);
		}
		else if(cmd == "reconfigure")
		{
			property_map new_props;
			if(!load_config(config_dir, "katina.conf", new_props))
				server.msg_to(num, "ERROR: Failed to reload config file");
			else
			{
				props.clear();
				props = new_props;
				server.msg_to(num, "Config file reloaded successfully");
			}
		}
		else if(cmd == "set")
		{
			// !katina plugin pluginid
			// !katina set varname value
			// -or-
			// !katina set pluginid::varname value
			str var, val;
			if(!(iss >> var >> val))
			{
				bug_var(val);
				log("Error: parsing builtin command parameters: " << text);
			}
			else
			{
				str p = plugin;

				if(siz pos = var.find("::") != str::npos)
				{
					p = var.substr(0, pos);
					if(var.size() < pos + 2)
					{
						server.msg_to(num, "ERROR varname not found");
						return;
					}
					var = var.substr(pos + 2);
				}

				if(!plugins[p])
					server.msg_to(num, "Plugin " + p + " is not loaded");
				else
				{
					if(!vars[plugins[p]][var])
						server.msg_to(num, "Plugin " + p + " does not recognise " + var);
					else
					{
						vars[plugins[p]][var]->set(val);
						if(vars[plugins[p]][var]->get(val))
							server.msg_to(num, "Variable " + p + "::" + var + " set to: " + val);
					}
				}
			}
		}
	}
}

bool Katina::initial_player_info()
{
	//bug_func();
	// ����      !listplayers: 9 players connected:
	//  2 S 5  Server Operator (*7B5DA741)   SooKee|AFK (a.k.a. SooKee)
	//  3 S 0   Unknown Player (*198025FD)   Kiki
	//  4 B 0   Unknown Player (*2671CC00)   Bruttler
	//  5 S 0   Unknown Player (*55EA82F1)   *M*^Mist1|AFK
	//  6 S 0   Unknown Player (*E2128F20)   Michel
	//  7 S 0   Unknown Player (*3DBE95A7)   Bloody [Boy]
	//  8 S 0   Unknown Player (*6122B476)   C@tch3r
	// 10 R 0   Unknown Player (*45B12012)   XZKTR
	// 11 S 5  Server Operator (*F587F42F)   a Cold Slug (a.k.a. Ximxim)

	str reply;
	if(!server.command("!listplayers", reply))
		if(!server.command("!listplayers", reply))
			return false;

	if(reply.find("!listplayers:") == str::npos)
	{
		log("ERROR: bad reply from !listplayers: " << reply);
		return false;
	}

	siz num;
	char team; // 1 = red, 2 = blue, 3 = spec

	siss iss(reply);
	str line, skip, guid, name;

	sgl(iss, line); // ignore first line

	while(sgl(iss, line))
	{
		//bug_var(line);
		siss iss(line);
		if(!sgl(sgl(iss >> num >> std::ws >> team, skip, '*'), guid, ')'))
		{
			log("ERROR: parsing !listplayers: " << line);
			continue;
		}

		if(num > 32)
		{
			log("ERROR: Bad client num: " << num);
			continue;
		}

		bug_var(num);
		bug_var(team);
		bug_var(guid);

		if(guid.size() != 8)
			clients[num] = GUID(num); // bot constructor
		else
			clients[num] = to<GUID>(guid);

		//bug("Adding: " << num << " to team " << team);
		bug_var(clients[num]);
		teams[clients[num]] = (team == 'R' ? 1 : (team == 'B' ? 2 : 3));
	}

	if(!server.command("status", reply))
		if(!server.command("status", reply))
			{ log("ERROR: No status"); return false; }


	if(reply.find("map:") == str::npos)
	{
		log("ERROR: bad reply from status: " << reply);
		return false;
	}

	// ����      map: am_thornish
	// num score ping name            lastmsg address               qport rate
	// --- ----- ---- --------------- ------- --------------------- ----- -----
	//   2     0   26 ^1S^2oo^3K^5ee^4|^7AFK^7     100 81.101.111.32         61881 25000
	//   3    46  138 Silent^7             50 96.241.187.112          814  2500
	//   4    32   98 ^2|<^8MAD^1Kitty Too^7       0 178.66.13.135         14639 25000

	iss.clear();
	iss.str(reply);

	bug_var(reply);

	const str term = "^7";

	while(sgl(iss, line) && line.find("---")) {}
//	bug("parse proper");
	while(sgl(iss, line))
	{
		if(trim(line).empty())
			continue;
//		bug_var(line);
		siss iss(line);
		if(!sgl(iss >> num >> skip >> skip >> std::ws, line))
		{
			log("ERROR: parsing status: " << line);
			continue;
		}
		// ^2|<^8MAD^1Kitty Too^7       0 178.66.13.135         14639 25000
		str_iter f = std::find_end(line.begin(), line.end(), term.begin(), term.end());
		if(f == line.end())
		{
			log("ERROR: parsing status name: " << line);
			continue;
		}
		players[clients[num]].assign(line.begin(), f).append(term);
		bug_var(num);
		bug_var(players[clients[num]]);

		iss.clear();
		iss.str(str(f, line.end()));

		str ip;
		if(iss >> skip >> std::ws >> skip >> std::ws >> ip)
			bug("IP ADDRESS: " << ip);

		// TODO: collect IPs in Katina (here too)
		//bug_var(players[clients[num]]);
	}

    return true;
}

// TODO: remove color codes
str sanitized(const str& name)
{
	return name;
}

bool Katina::parse_slot_guid_name(const str& slot_guid_name, slot& num)
{
	// 12 | A0B65FD9 | wibble

	slot n;

	if(slot_guid_name.size() > 2 && slot_guid_name.size() < 8) // try GUID startswith
		for(guid_siz_map_citer i = teams.begin(); i != teams.end(); ++i)
			if(!upper_copy(str(i->first)).find(upper_copy(slot_guid_name)))
				if((n = getClientSlot(i->first)) != bad_slot)
					return (num = n) != bad_slot;

	if(slot_guid_name.size() > 3) // try name submatch
		for(guid_str_map_citer i = players.begin(); i != players.end(); ++i)
			if(sanitized(i->second).find(lower_copy(slot_guid_name)) != str::npos)
				if((n = getClientSlot(i->first)) != bad_slot)
					return (num = n) != bad_slot;

	siss iss(slot_guid_name);
	if(slot_guid_name.size() < 3) // try slot match
		if(iss >> n && check_slot(n))
			return (num = n) != bad_slot;

	// try exact name match
	for(guid_str_map_citer i = players.begin(); i != players.end(); ++i)
		if(sanitized(i->second) == lower_copy(slot_guid_name))
			if((n = getClientSlot(i->first)) != bad_slot)
				return (num = n) != bad_slot;

	return false;
}

bool Katina::start(const str& dir)
{
	log("Starting Katina:");
	config_dir = expand_env(dir);
	log("Setting config dir: " << dir);

	load_config(config_dir, "katina.conf", props);

	name = get("katina.name" , "^1K^7at^3i^7na^7");

    if(!init_pki())
    	return false;
    init_rcon();

    // everything the plugins need shuld be initialized before loading them

	// Get mod_katina version if available
	if(!rconset("mod_katina", mod_katina))
		if(!rconset("mod_katina", mod_katina))
			mod_katina.clear();

	if(!initial_player_info())
		log("ERROR: Unable to get initial player info");

	now = get("run.time", std::time(0));
	std::time_t base_now = now; // rerun base time

	bug_var(now);

    load_plugins();

	std::ios::openmode mode = std::ios::in|std::ios::ate;

	bool rerun = get("run.mode") == "rerun";
	if(rerun)
		mode = std::ios::in;

	log("Opening logfile (" << get("run.mode") << "): " << get("logfile"));

	std::ifstream ifs;
	ifs.open(get_exp("logfile").c_str(), mode);

	if(!ifs.is_open())
	{
		log("FATAL: Logfile not found: " << get("logfile"));
		return false;
	}

	std::istream& is = ifs;
	std::ios::streampos gpos = is.tellg();

	// Sometimes the ClientUserinfoChanged message is split over
	// two lines. This is a fudge to compensate for that bug
	struct client_userinfo_bug_t
	{
		str params;
		void set(const str& params) { this->params = params; }
		void reset() { params.clear(); }
		operator bool() { return !params.empty(); }
	} client_userinfo_bug;

	client_userinfo_bug.reset();

	log("Processing:");

	//===================================================//
	//= MAIN LOOP                                       =//
	//===================================================//

	// working variables
	char c;
	siz min, sec;
	str line, skip, name, cmd;
	siss iss;

	while(!done)
	{
		if(!std::getline(is, line) || is.eof())
		{
			if(rerun)
				done = true;
			thread_sleep_millis(100);
			is.clear();
			is.seekg(gpos);
			continue;
		}

		gpos = is.tellg();

		if(!active)
			continue;

		iss.clear();
		iss.str(line);

		if(!(sgl(iss >> min >> c >> sec >> std::ws, cmd, ':') >> std::ws))
		{
			if(!client_userinfo_bug)
			{
				log("ERROR: parsing logfile command: " << line);
				continue;
			}
			log("WARN: possible ClientUserinfoChanged bug");
			if(line.find("\\id\\") == str::npos)
			{
				log("ERROR: parsing logfile command: " << line);
				client_userinfo_bug.reset();
				continue;
			}
			else
			{
				log("INFO: ClientUserinfoChanged bug detected");
				cmd = "ClientUserinfoChanged";
				iss.clear();
				iss.str(client_userinfo_bug.params + line);
				log("INFO: params: " << client_userinfo_bug.params << line);
			}
		}

		client_userinfo_bug.reset();

		if(!cmd.find("----"))
		{
			if(!min && !sec)
			{
				base_now = std::time(0);
				//base_now = now;
				bug("=========================");
				bug("= BASE_TIME: " << base_now << " =");
				bug("=========================");
			}
			continue;
		}

		cmd += ":";

		str params;

		sgl(iss, params); // not all commands have params

		iss.clear();
		iss.str(params);

		//lock_guard lock(cvarevts_mtx);

		//if(rerun)
			now = base_now + (min * 60) + sec;
		//else
		//	now = std::time(0);
        
        // Send HEARTBEAT event to plugins
        for(plugin_vec_iter i = events[HEARTBEAT].begin(); i != events[HEARTBEAT].end(); ++i)
            (*i)->heartbeat(min, sec);

        bool flagspeed = false; // speed carrying a flag

		if(cmd == "Exit:")
		{
			//bug(cmd << "(" << params << ")");
			if(events[EXIT].empty())
				continue;

			for(plugin_vec_iter i = events[EXIT].begin()
				; i != events[EXIT].end(); ++i)
				(*i)->exit(min, sec);
		}
		else if(cmd == "ShutdownGame:")
		{
			for(plugin_vec_iter i = events[SHUTDOWN_GAME].begin()
				; i != events[SHUTDOWN_GAME].end(); ++i)
				(*i)->shutdown_game(min, sec);

			// these are clients that disconnected before the game ended
			for(const GUID& guid: shutdown_erase)
			{
				teams.erase(guid);
				players.erase(guid);
			}
			shutdown_erase.clear();
		}
		else if(cmd == "Warmup:")
		{
			//bug(cmd << "(" << params << ")");
			if(events[WARMUP].empty())
				continue;

			for(plugin_vec_iter i = events[WARMUP].begin()
				; i != events[WARMUP].end(); ++i)
				(*i)->warmup(min, sec);
		}
		else if(cmd == "ClientUserinfoChanged:")
		{
			//bug(cmd << "(" << params << ")");
			// 0 n\Merman\t\2\model\merman\hmodel\merman\c1\1\c2\1\hc\70\w\0\l\0\skill\ 2.00\tt\0\tl\0\id\
			// 2 \n\^1S^2oo^3K^5ee\t\3\c2\d\hc\100\w\0\l\0\tt\0\tl\0\id\041BD1732752BCC408FAF45616A8F64B
			siz num, team;
			if(!(sgl(sgl(sgl(iss >> num, skip, '\\'), name, '\\'), skip, '\\') >> team))
				std::cout << "Error parsing ClientUserinfoChanged: "  << params << '\n';
			else if(num > 32)
			{
				log("ERROR: Client num too high: " << num);
			}
			else
			{
				siz pos = line.find("\\id\\");
				if(pos == str::npos)
					client_userinfo_bug.set(params);
				else
				{
					str id = line.substr(pos + 4, 32);
					GUID guid;

					if(id.size() != 32)
						guid = GUID(num); // bot constructor
					else
						guid = to<GUID>(id.substr(24));

					siz hc = 100;
					if((pos = line.find("\\hc\\")) == str::npos)
					{
						log("WARN: no handicap info found: " << line);
					}
					else
					{
						if(!(siss(line.substr(pos + 4)) >> hc))
							log("ERROR: Parsing handicap: " << line.substr(pos + 4));
					}

					shutdown_erase.remove(guid); // must have re-joined

					clients[num] = guid;
					players[guid] = name;
                    
                    siz teamBefore = teams[guid];
                    teams[guid] = team; // 1 = red, 2 = blue, 3 = spec

					for(plugin_vec_iter i = events[CLIENT_USERINFO_CHANGED].begin();
							i != events[CLIENT_USERINFO_CHANGED].end(); ++i)
						(*i)->client_userinfo_changed(min, sec, num, team, guid, name, hc);
                    
                    if(team != teamBefore && !guid.is_bot())
                        for(plugin_vec_iter i = events[CLIENT_SWITCH_TEAM].begin();
                        		i != events[CLIENT_SWITCH_TEAM].end(); ++i)
                            (*i)->client_switch_team(min, sec, num, teamBefore, team);
				}
			}
		}
		else if(cmd == "ClientConnect:")
		{
			if(events[CLIENT_CONNECT].empty())
				continue;

			siz num;
			if(!(iss >> num))
				log("Error parsing ClientConnect: "  << params);
			else
			{
				for(plugin_vec_iter i = events[CLIENT_CONNECT].begin()
					; i != events[CLIENT_CONNECT].end(); ++i)
					(*i)->client_connect(min, sec, num);
			}
		}
		else if(cmd == "ClientConnectInfo:")
		{
			// ClientConnectInfo: 4 87597A67B5A4E3C79544A72B7B5DA741 81.101.111.32
			if(events[CLIENT_CONNECT_INFO].empty())
				continue;

			siz num;
			GUID guid;
			str ip;
			str skip; // rest of guid needs to be skipped before ip

			if(!(iss >> num >> std::ws >> guid >> skip >> std::ws >> ip))
				log("Error parsing ClientConnectInfo: "  << params);
			else
			{
				for(plugin_vec_iter i = events[CLIENT_CONNECT_INFO].begin()
					; i != events[CLIENT_CONNECT_INFO].end(); ++i)
					(*i)->client_connect_info(min, sec, num, guid, ip);
			}
		}
		else if(cmd == "ClientBegin:") // 0:04 ClientBegin: 4
		{
			//bug(cmd << "(" << params << ")");
			if(events[CLIENT_BEGIN].empty())
				continue;

			siz num;
			if(!(iss >> num))
				log("Error parsing ClientBegin: "  << params);
			else
			{
				for(plugin_vec_iter i = events[CLIENT_BEGIN].begin()
					; i != events[CLIENT_BEGIN].end(); ++i)
					(*i)->client_begin(min, sec, num);
			}
		}
		else if(cmd == "ClientDisconnect:")
		{
			//bug(cmd << "(" << params << ")");

			siz num;
			if(!(iss >> num))
				std::cout << "Error parsing ClientDisconnect: "  << params << '\n';
			else if(num > 32)
			{
				log("ERROR: Client num too high: " << num);
			}
			else
			{
				for(plugin_vec_iter i = events[CLIENT_DISCONNECT].begin()
					; i != events[CLIENT_DISCONNECT].end(); ++i)
					(*i)->client_disconnect(min, sec, num);

				// slot numbers are defunct, but keep GUIDs until ShutdownGame
				shutdown_erase.push_back(clients[num]);

 				//teams.erase(clients[num]);
				//players.erase(clients[num]);
				clients.erase(num);
			}
		}
		else if(cmd == "Kill:")
		{
			//bug(cmd << "(" << params << ")");
			if(events[KILL].empty())
				continue;

			siz num1, num2, weap;
			if(!(iss >> num1 >> num2 >> weap))
				log("Error parsing Kill:" << params);
			else
			{
				for(plugin_vec_iter i = events[KILL].begin()
					; i != events[KILL].end(); ++i)
					(*i)->kill(min, sec, num1, num2, weap);
			}
		}
		else if(cmd == "Push:") // mod_katina only
		{
			//bug(cmd << "(" << params << ")");
			if(events[PUSH].empty())
				continue;

			siz num1, num2;
			if(!(iss >> num1 >> num2))
				log("Error parsing Push:" << params);
			else
			{
				for(plugin_vec_iter i = events[PUSH].begin()
					; i != events[PUSH].end(); ++i)
					(*i)->push(min, sec, num1, num2);
			}
		}
		else if(cmd == "WeaponUsage:")
		{
			//bug(cmd << "(" << params << ")");

			// Weapon Usage Update
			// WeaponUsage: <client#> <weapon#> <#shotsFired>
			siz num, weap, shots;

			if(iss >> num >> weap >> shots)
			{
				for(plugin_vec_iter i = events[WEAPON_USAGE].begin(); i != events[WEAPON_USAGE].end(); ++i)
					(*i)->weapon_usage(min, sec, num, weap, shots);
			}
			else
				log("Error parsing WeaponUsage: " << params);
		}
		else if(cmd == "MODDamage:")
		{
			//bug(cmd << "(" << params << ")");

			// MOD (Means of Death = Damage Type) Damage Update
			// MODDamage: <client#> <mod#> <#hits> <damageDone> <#hitsRecv> <damageRecv> <weightedHits>
			siz num, mod, hits, dmg, hitsRecv, dmgRecv;
			float weightedHits;
			if(iss >> num >> mod >> hits >> dmg >> hitsRecv >> dmgRecv >> weightedHits)
			{
				for(plugin_vec_iter i = events[MOD_DAMAGE].begin(); i != events[MOD_DAMAGE].end(); ++i)
					(*i)->mod_damage(min, sec, num, mod, hits, dmg, hitsRecv, dmgRecv, weightedHits);
			}
			else
				log("Error parsing MODDamage: " << params);
		}
		else if(cmd == "PlayerStats:")
		{
			//bug(cmd << "(" << params << ")");

			// Player Stats Update
			// PlayerStats: <client#>
			// 			    <fragsFace> <fragsBack> <fraggedInFace> <fraggedInBack>
			// 			    <spawnKillsDone> <spanwKillsRecv>
			// 			    <pushesDone> <pushesRecv>
			// 			    <healthPickedUp> <armorPickedUp>
			//				<holyShitFrags> <holyShitFragged>
			siz num, fragsFace, fragsBack, fraggedFace, fraggedBack, spawnKills, spawnKillsRecv;
			siz pushes, pushesRecv, health, armor, holyShitFrags, holyShitFragged;
			if(iss >> num >> fragsFace >> fragsBack >> fraggedFace >> fraggedBack >> spawnKills >> spawnKillsRecv
			       >> pushes >> pushesRecv >> health >> armor >> holyShitFrags >> holyShitFragged)
			{
				for(plugin_vec_iter i = events[PLAYER_STATS].begin(); i != events[PLAYER_STATS].end(); ++i)
				{
					(*i)->player_stats(min, sec, num,
						fragsFace, fragsBack, fraggedFace, fraggedBack,
						spawnKills, spawnKillsRecv, pushes, pushesRecv,
						health, armor, holyShitFrags, holyShitFragged);
				}
			}
			else
				log("Error parsing PlayerStats: " << params);
		}
		else if(cmd == "CTF:")
		{
			if(events[CTF].empty())
				continue;

			//bug(cmd << "(" << params << ")");

			siz num, col, act;
			if(!(iss >> num >> col >> act) || col < 1 || col > 2)
				log("Error parsing CTF:" << params);
			else
			{
				for(plugin_vec_iter i = events[CTF].begin()
					; i != events[CTF].end(); ++i)
					(*i)->ctf(min, sec, num, col, act);
			}
		}
		else if(cmd == "red:") // BUG: red:(8  blue:6) [Katina.cpp] (662)
		{
			//bug(cmd << "(" << params << ")");
			if(events[CTF_EXIT].empty())
				continue;

			//bug_var(iss.str());
			siz r = 0;
			siz b = 0;
			str skip;
			if(!(sgl(iss >> r >> std::ws, skip, ':') >> b))
				log("Error parsing CTF_EXIT:" << params);
			else
			{
//				bug_var(r);
//				bug_var(skip);
//				bug_var(b);
				for(plugin_vec_iter i = events[CTF_EXIT].begin()
					; i != events[CTF_EXIT].end(); ++i)
					(*i)->ctf_exit(min, sec, r, b);
			}
		}
		else if(cmd == "score:") //
		{
			//bug(cmd << "(" << params << ")");
			if(events[SCORE_EXIT].empty())
				continue;

			int score = 0;
			siz ping = 0;
			siz num = 0;
			str name;
			// 18:38 score: 200  ping: 7  client: 0 ^5A^6lien ^5S^6urf ^5G^6irl
			// 18:38 score: 196  ping: 65  client: 5 ^1Lord ^2Zeus
			// 18:38 score: 121  ping: 200  client: 1 (drunk)Mosey
			// 18:38 score: 115  ping: 351  client: 2 Wark
			// 18:38 score: 102  ping: 315  client: 3 Next map
			// 18:38 score: 89  ping: 235  client: 4 ^1S^3amus ^1A^3ran
			// 18:38 score: 30  ping: 228  client: 6 ^1LE^0O^4HX
			// 18:38 score: 6  ping: 50  client: 7 Cyber_Ape
			if(!sgl(iss >> score >> skip >> ping >> skip >> num >> std::ws, name))
				log("Error parsing SCORE_EXIT:" << params);
			else
			{
//				bug_var(score);
//				bug_var(ping);
//				bug_var(num);
//				bug_var(name);
				for(plugin_vec_iter i = events[SCORE_EXIT].begin()
					; i != events[SCORE_EXIT].end(); ++i)
					(*i)->score_exit(min, sec, score, ping, num, name);
			}
 		}
		else if((flagspeed = cmd == "SpeedFlag:") || (cmd == "Speed:")) // mod_katina >= 0.1-beta
		{
			// 9:35 Speed: 3 1957 13 : Client 3 ran 1957u in 13s without the flag.
			// 9:35 SpeedFlag: 3 3704 12 : Client 3 ran 3704u in 12s while holding the flag.
			//bug(cmd << "(" << params << ")");
			if(events[SPEED].empty())
				continue;

			siz num, dist, time;
			if(!(iss >> num >> dist >> time))
				log("Error parsing Speed:" << params);
			else
			{
				for(plugin_vec_iter i = events[SPEED].begin()
					; i != events[SPEED].end(); ++i)
					(*i)->speed(min, sec, num, dist, time, flagspeed);
			}
		}
		else if(cmd == "Award:")
		{
			//bug(cmd << "(" << params << ")");
			if(events[AWARD].empty())
				continue;

			siz num, awd;
			if(!(iss >> num >> awd))
				log("Error parsing Award:" << params);
			else
			{
				for(plugin_vec_iter i = events[AWARD].begin()
					; i != events[AWARD].end(); ++i)
					(*i)->award(min, sec, num, awd);
			}
		}
		else if(cmd == "InitGame:")
		{
			//bug(cmd << "(" << params << ")");

			static str key, val;

//			clients.clear();
//			players.clear();
//			teams.clear();
			svars.clear();

			iss.ignore(); // skip initial '\\'
			while(sgl(sgl(iss, key, '\\'), val, '\\'))
				svars[key] = val;

			mapname = svars["mapname"];

			if(rerun)
			{
				str skip;
				siz Y, M, D, h, m, s;
				char c;
				siss iss(svars["g_timestamp"]);
				// g_timestamp 2013-05-24 09:34:32
				if((iss >> Y >> c >> M >> c >> D >> c >> h >> c >> m >> c >> s))
				{
					tm t;
					std::time_t _t = std::time(0);
					t = *gmtime(&_t);
					t.tm_year = Y - 1900;
					t.tm_mon = M;
					t.tm_mday = D;
					t.tm_hour = h;
					t.tm_min = m;
					t.tm_sec = s;
					t.tm_isdst = 0;
					base_now = std::mktime(&t);
					log("RERUN TIMESTAMP: " << base_now);
				}
			}

			str msg = this->name + " ^3Stats System v^7" + version + (tag.size()?"^3-^7":"") + tag;
			server.cp(msg);

			log("MAP NAME: " << mapname);

			if(events[INIT_GAME].empty())
				continue;

			for(plugin_vec_iter i = events[INIT_GAME].begin()
				; i != events[INIT_GAME].end(); ++i)
				(*i)->init_game(min, sec, svars);
		}
		else if(cmd == "Playerstore:")
		{
		}
		else if(cmd == "Restored")
		{
		}
		else if(cmd == "PlayerScore:")
		{
		}
		else if(cmd == "Challenge:")
		{
		}
		else if(cmd == "Info:")
		{
		}
		else if(cmd == "Item:")
		{
		}
		else if(cmd == "score:")
		{
		}
		else if(cmd == "Callvote:") // mod_katina >= 0.1-beta
		{
			//   2:14 Callvote: 0 custom nobots: ^1S^2oo^3K^5ee^4|^7AFK has called a vote for 'custom nobots'
			if(events[LOG_CALLVOTE].empty())
				continue;

			siz num;
			str type;
			str info;

			if(!sgl(iss >> num >> std::ws >> type >> std::ws, info, ':'))
				log("Error parsing Callvote: "  << params);
			else
			{
				for(plugin_vec_iter i = events[LOG_CALLVOTE].begin()
					; i != events[LOG_CALLVOTE].end(); ++i)
					(*i)->callvote(min, sec, num, type, info);
			}
		}
		else if(cmd == "sayteam:")
		{
			if(events[SAYTEAM].empty())
				continue;

			//bug(cmd << "(" << params << ")");

			str text;
			GUID guid;

			if(extract_name_from_text(line, guid, text))
				for(plugin_vec_iter i = events[SAYTEAM].begin()
					; i != events[SAYTEAM].end(); ++i)
					(*i)->sayteam(min, sec, guid, text);
		}
		else if(cmd == "say:")
		{
			if(events[SAY].empty())
				continue;

			str text;
			GUID guid;

			if(extract_name_from_text(line, guid, text))
			{
				if(!text.find("!katina"))
					builtin_command(guid, text);
				else
					for(plugin_vec_iter i = events[SAY].begin()
						; i != events[SAY].end(); ++i)
						(*i)->say(min, sec, guid, text);
			}
		}
		else if(cmd == "chat:")
		{
			if(events[CHAT].empty())
				continue;

			for(plugin_vec_iter i = events[CHAT].begin()
				; i != events[CHAT].end(); ++i)
				(*i)->chat(min, sec, params);
		}
		else
		{
			if(events[UNKNOWN].empty())
				continue;

			//bug("UNKNOWN: " << cmd << "(" << params << ")");

			for(plugin_vec_iter i = events[UNKNOWN].begin()
				; i != events[UNKNOWN].end(); ++i)
				(*i)->unknown(min, sec, cmd, params);
		}
		//pthread_mutex_unlock(&cvarevts_mtx);
	}
    
	return true;
}

} // oastats
