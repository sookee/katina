/*
 * File:   Katina.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 1, 2013, 6:23 PM
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@gmail.com			   |
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
#include <string>

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

namespace katina {

using namespace katina::log;
using namespace katina::net;
using namespace katina::time;
using namespace katina::types;
using namespace katina::utils;
using namespace katina::string;

const str version = "0.1";
const str tag = "";
const str revision = REV;

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
		return "";

	guid_str_map_citer p;
	if((p = players.find(c->second)) == players.cend())
		return "";

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
	for(slot_guid_map_citer it = clients.cbegin(); it != clients.cend(); ++it)
		if(it->second == guid)
			return it->first;
	return slot::bad;
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
//	GUID candidate;
	siz pos = 0;
	siz beg = 0;
	if((beg = line.find(": ")) == str::npos) // "say: "
		return false;

	beg += 2;

	bool found = false;
	for(pos = beg; (pos = line.find(": ", pos)) != str::npos; pos += 2)
	{
		GUID candidate(guid_from_name(line.substr(beg, pos - beg)));
		if(candidate == null_guid)
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

//	if(!plugin->open())
//	{
//		log("PLUGIN LOAD: plugin failed to open");
//		delete plugin;
//		if(dlclose(dl))
//			log("PLUGIN LOAD: plugin failed to unload: " << dlerror());
//		return false;
//	}
//
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

	// remove vars
	cvar_map_map_iter v = vars.find(i->second);
	if(v != vars.end())
		vars.erase(v);

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

	if(!open_plugin(id))
	{
		log("PLUGIN RELOAD: ERROR: plugin '" << id << "' failed to reopen: " << f->second);
		return false;
	}

	return true;
}

bool Katina::open_plugin(const str& id)
{
	log("PLUGIN OPEN: " << id);

	plugin_map_iter p = plugins.find(id);

	if(p == plugins.end())
	{
		log("PLUGIN OPEN: ERROR: plugin file not known: " << id);
		return false;
	}

	if(!p->second->open())
	{
		log("PLUGIN OPEN: ERROR: plugin failed to open: " << id);
		return false;
	}

	p->second->opened = true;

	log("PLUGIN OPEN: OK: " << p->second->get_id() << ", " << p->second->get_name() << ", " << p->second->get_version());

	return true;
}

void Katina::load_plugins()
{
	log("Loading plugins:");
	str_vec pluginfiles = get_exp_vec("plugin");
	for(siz i = 0; i < pluginfiles.size(); ++i)
		load_plugin(pluginfiles[i]);

	log("Opening plugins:");
	for(plugin_map_iter p = plugins.begin(); p != plugins.end();)
	{
		if(open_plugin(p->first))
			++p;
		else
		{
			delete p->second;
			p = plugins.erase(p);
		}
	}
}

KatinaPlugin* Katina::get_plugin(const str& id, const str& version)
{
	plugin_map_iter i = plugins.find(id);

	if(i == plugins.end())
	{
		log("ERROR: plugin not found: " << id);
		return nullptr;
	}

	if(i->second->get_version() < version)
	{
		log("ERROR: wrong version found: " << i->second->get_version() << " expected " << version);
		return nullptr;
	}

	return i->second;
}

bool Katina::chat_to(slot num, const str& text)
{
	return chat_to(getClientGuid(num), text);
}

bool Katina::chat_to(const GUID& guid, const str& text)
{
	return chat_to(getPlayerName(guid), text);
}

bool Katina::chat_to(const str& name, const str& text)
{
	return server.s_chat(name + "^2 " + text);
}

bool Katina::is_admin(const GUID& guid)
{
	bug_func();
	bug_var(guid);
	bug_var(getPlayerName(guid));
	str_vec admins = get_vec("admin.guid");
	for(str_vec_iter i = admins.begin(); i != admins.end(); ++i)
		if(guid == GUID(*i))
			return true;

	// now try admin.dat file
	str admin_dat = get_exp("admin.dat.file");
	if(admin_dat.empty())
		return false;
	sifs ifs(admin_dat.c_str());
	if(!ifs)
	{
		log("WARN: admin.dat file not found: " << admin_dat);
		return false;
	}

	str line;
	// [admin]
	// name	= ^1S^2oo^3K^5ee
	// guid	= 87597A67B5A4E3C79544A72B7B5DA741
	while(sgl(ifs, line))
		if(trim(line) == "[admin]")
			if(sgl(sgl(ifs, line), line))
				if(trim(line).size() == 32 && guid == GUID(line.substr(0, 8)))
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
			log("ERROR:					: " << line);
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

	str keypair_file = get_exp("pki.keypair", "katina.pki");
	str pub_key_file = get("pki.pub.key", "katina.pub");

	if(!pki.load_keypair(config_dir + "/" + keypair_file))
	{
		log("WARNING: Unable to load keypair: " << keypair_file);
		log("	   : Creating a new one");

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
	}

	// always write out the public key from the keypair
	// so they always match
	log("Outputting public key: " << pub_key_file);
	str sexp;
	if(pki.get_public_key_as_text(sexp))
	{
		std::ofstream ofs((config_dir + "/" + pub_key_file).c_str());
		ofs << sexp;
	}

	log("Scanning for public keys:");

	const str_vec& keys = get_vec("pki.remote.key");

	for(const str& key: keys)
	{
		str id, file;
		if(!(siss(key) >> id >> file))
		{
			log("WARN: error parsing key entry: " << key);
			continue;
		}
		log("Reading public keys: [" << id << "] " << file);
		pki.add_client_key_file(id, expand_env(file, WRDE_SHOWERR|WRDE_UNDEF));
	}

	return true;
}

void Katina::init_rcon()
{
	log("Initializing rcon:");
	server.config(get("rcon.host"), get<siz>("rcon.port"), get("rcon.pass"));

	if(get("rcon.active") == "true")
		server.on();
}

void Katina::builtin_command(const GUID& guid, const str& text)
{
	bug_func();
	bug_var(guid);
	bug_var(text);

	slot num;

	if((num = getClientSlot(guid)) == slot::bad)
	{
		server.s_chat("ERROR: Cannot locate client number.");
		return;
	}

	if(trim_copy(text).empty())
	{
		server.msg_to(num, name + "^7: " + revision);
		return;
	}

	if(!is_admin(guid))
	{
		server.msg_to(num, name + "^7: " + "^3You need to be admin to use this");
		return;
	}

	str cmd;
	siss iss(text);

	static const siz WIDTH = 70;

	if(!(iss >> cmd >> cmd >> std::ws))
		log("Error: parsing builtin command: " << text);
	else
	{
		bug_var(cmd);
		if(cmd == "debug" || cmd == "-d")
		{
			str type;

			if(!(iss >> type) || trim(type).empty())
				type = "data";

			soss oss;

			if(type == "clients" || type == "data")
			{
				server.msg_to(num, "clients: " + std::to_string(clients.size()));
				oss.str("");
				for(slot_guid_map_vt p: clients)
				{
					oss << "{";
					oss << (p.second.is_connected()?"C":"D");
					oss << (p.second.is_bot()?"B":"H");
					oss <<  ":" + str(siz(p.first)<10?" ":"") + str(p.first) + "," + str(p.second);
					oss << "}";

					if(oss.str().size() > 80)
					{
						server.msg_to(num, oss.str());
						oss.str("");
					}
				}
				if(!oss.str().empty())
					server.msg_to(num, oss.str());
			}
			if(type == "teams" || type == "data")
			{
				TYPEDEF_MMAP(siz, GUID, siz_guid_mmap);

				siz_guid_mmap sorted;

				for(const guid_siz_map_vt& p: teams)
					sorted.insert({p.second, p.first});

				server.msg_to(num, "teams: " + std::to_string(teams.size()));

				oss.str("");
				for(const siz_guid_mmap_vt& p: sorted)
				{
					oss << "{";
					oss << (p.second.is_connected()?"C":"D");
					oss << (p.second.is_bot()?"B":"H");
					oss << ":" + std::to_string(p.first) + "," + str(p.second);
					oss << "}";

					if(oss.str().size() > WIDTH)
					{
						server.msg_to(num, oss.str());
						oss.str("");
					}
				}
				if(!oss.str().empty())
					server.msg_to(num, oss.str());
			}
			if(type == "players" || type == "data")
			{
				server.msg_to(num, "players: " + std::to_string(players.size()));
				oss.str("");
				for(const guid_str_map_vt& p: players)
				{
					oss << "{";
					oss << (p.first.is_connected()?"C":"D");
					oss << (p.first.is_bot()?"B":"H");
					oss << ":" + str(p.first) + ", " + p.second + "^7";
					oss << "}";

					if(oss.str().size() > WIDTH)
					{
						server.msg_to(num, oss.str());
						oss.str("");
					}
				}
				if(!oss.str().empty())
					server.msg_to(num, oss.str());
			}
			if(type == "connected" || type == "data")
			{
				siz c = std::count_if(connected.begin(), connected.end(), [](const bool& b){return b;});
				server.msg_to(num, "connected: " + std::to_string(c));
				oss.str("");
				c = 0;
				for(bool b: connected)
				{
					if(b)
					{
						oss << "{" + std::to_string(c) + ": " + std::to_string(b) + "}";

						if(oss.str().size() > WIDTH)
						{
							server.msg_to(num, oss.str());
							oss.str("");
						}
					}
					++c;
				}
				if(!oss.str().empty())
					server.msg_to(num, oss.str());
			}
		}
		else if(cmd == "plugin")
		{
			if(!(iss >> plugin))
			{
				// list plugins
				server.msg_to(num, "available plugins:");
				for(const plugin_map_vt& p: plugins)
					server.msg_to(num, " " + p.first);
				return;
			}

			if(plugins.find(plugin) == plugins.end())
			{
				// list plugins
				server.msg_to(num, "unknown plugin: ^1" + plugin + "^7:");
				for(const plugin_map_vt& p: plugins)
					server.msg_to(num, " " + p.first);
				return;
			}

			server.msg_to(num, "plugin is now: " + plugin);
		}
		else if(cmd == "plugins")
		{
			server.msg_to(num, "available plugins:");
			for(const plugin_map_vt& p: plugins)
				server.msg_to(num, " " + p.first);
			return;
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
			//
			// !katina set [pluginid::]varname value // set variable
			// !katina set [pluginid::]varname	 // display value
			// !katina set [pluginid::]			// display list of variables

			bool show_list = false;
			bool show_value = false;

			str p = plugin;

			str var, val;
			if(!(iss >> var)) // list selected plugin (p)
				show_list = true;
			else
			{
				// resolve plugin p & variable var
				if(plugins.find(var) != plugins.end())
				{
					p = var;
					var.clear();
					show_list = true;
				}
				else
				{
					siz pos;
					if((pos = var.rfind("::")) != str::npos)
					{
						con("pos: " << pos);
						p = var.substr(0, pos);
						if(var.size() < pos + 2)
							show_list = true; // list plugin p
						else
							var = var.substr(pos + 2);
					}

					if(plugins.find(p) == plugins.end())
					{
						server.msg_to(num, "Plugin " + p + " is not registered");
						return;;
					}

					if(!plugins[p])
					{
						server.msg_to(num, "Plugin " + p + " is not loaded");
						return;
					}

					// recognize variable var
					if(vars.find(plugins[p]) == vars.end())
					{
						server.msg_to(num, "Plugin " + p + " is not registered");
						return;
					}

					if(vars[plugins[p]].find(var) == vars[plugins[p]].end())
					{
						server.msg_to(num, "Plugin " + p + " does not recognize variable '" + var + "'");
						return;
					}

					if(!(iss >> val)) // list || display
						show_value = true;
				}
			}

			if(plugins.find(p) == plugins.end())
			{
				server.msg_to(num, "Plugin " + p + " is not registered");
				return;
			}

			if(!plugins[p])
			{
				server.msg_to(num, "Plugin " + p + " is not loaded");
				return;
			}

			if(show_list)
			{
				for(const cvar_map_pair& v: vars[plugins[p]])
					if(v.second && v.second->get(val))
						server.msg_to(num, v.first + ": " + val);
			}
			else if(show_value)
			{
				if(vars[plugins[p]][var]->get(val))
					server.msg_to(num, var + ": " + val);
			}
			else
			{
				vars[plugins[p]][var]->set(val);
				if(vars[plugins[p]][var]->get(val))
				{
					server.msg_to(num, p + "::" + var + ": " + val);
					log("set cvar: " << p << "::" << var << ": " << val << " [" << getPlayerName(guid) << "]");
				}
			}
		}
	}
}

bool Katina::initial_player_info()
{
	//bug_func();
	// ����	  !listplayers: 9 players connected:
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

	slot num;
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

		if(num >= slot::max)
		{
			log("ERROR: Bad client num: " << num);
			continue;
		}

		bug_var(num);
		bug_var(team);
		bug_var(guid);

		if(guid.size() != 8)
		{
			bug("BOT FOUND: " << num);
			clients[num] = GUID(num); // bot constructor
			if(!clients[num].is_bot())
				log("ERROR: not set to bot");
		}
		else
		{
			bug("HUMAN FOUND: " << num);
			clients[num] = GUID(guid);
			if(clients[num].is_bot())
				log("ERROR: set to bot");
		}

		//bug("Adding: " << num << " to team " << team);
		bug_var(getClientGuid(num));
		teams[getClientGuid(num)] = (team == 'R' ? 1 : (team == 'B' ? 2 : 3));
	}

	if(!server.command("status", reply))
		if(!server.command("status", reply))
			{ log("ERROR: No status"); return false; }


	if(reply.find("map:") == str::npos)
	{
		log("ERROR: bad reply from status: " << reply);
		return false;
	}

	// ����	  map: am_thornish
	// num score ping name			lastmsg address			   qport rate
	// --- ----- ---- --------------- ------- --------------------- ----- -----
	//   2	 0   26 ^1S^2oo^3K^5ee^4|^7AFK^7	 100 81.101.111.32		 61881 25000
	//   3	46  138 Silent^7			 50 96.241.187.112		  814  2500
	//   4	32   98 ^2|<^8MAD^1Kitty Too^7	   0 178.66.13.135		 14639 25000

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
		// ^2|<^8MAD^1Kitty Too^7	   0 178.66.13.135		 14639 25000
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

	slot s;

	if(slot_guid_name.size() > 2 && slot_guid_name.size() < 8) // try GUID startswith
		for(guid_siz_map_citer i = teams.begin(); i != teams.end(); ++i)
			if(!upper_copy(str(i->first)).find(upper_copy(slot_guid_name)))
				if((s = getClientSlot(i->first)) != slot::bad)
					return (num = s) != slot::bad;

	if(slot_guid_name.size() > 3) // try name submatch
		for(guid_str_map_citer i = players.begin(); i != players.end(); ++i)
			if(sanitized(i->second).find(lower_copy(slot_guid_name)) != str::npos)
				if((s = getClientSlot(i->first)) != slot::bad)
					return (num = s) != slot::bad;

	siss iss(slot_guid_name);
	if(slot_guid_name.size() < 3) // try slot match
		if(iss >> s && check_slot(s))
			return (num = s) != slot::bad;

	// try exact name match
	for(guid_str_map_citer i = players.begin(); i != players.end(); ++i)
		if(sanitized(i->second) == lower_copy(slot_guid_name))
			if((s = getClientSlot(i->first)) != slot::bad)
				return (num = s) != slot::bad;

	return false;
}

// Sometimes the ClientUserinfoChanged message is split over
// two lines. This is a fudge to compensate for that bug
struct client_userinfo_bug_t
{
	str params;
	void set(const str& params) { this->params = params; }
	void reset() { params.clear(); }
	operator bool() { return !params.empty(); }
};


bool Katina::read_backlog(const str& logname, std::ios::streampos pos)
{
	bug_func();
	nlog("pos: " << pos);

	sifs ifs(logname);

	client_userinfo_bug_t client_userinfo_bug;

	siss iss;
	siz min, sec;
	char c;
	str cmd;
	str skip;
	str name;

	line_number = 0;
	while(sgl(ifs, line_data))
	{
		if(ifs.tellg() >= pos)
			return true;

		++line_number; // current line number
		if(do_log_lines)
			nlog("LINE: " << line_data);

		if(trim(line_data).empty())
			continue;

		iss.clear();
		iss.str(line_data);

		if(!(sgl(iss >> min >> c >> sec >> std::ws, cmd, ':') >> std::ws))
		{
			if(!client_userinfo_bug)
			{
				nlog("ERROR: parsing logfile command: " << line_data);
				continue;
			}
			log("WARN: possible ClientUserinfoChanged bug");
			if(line_data.find("\\id\\") == str::npos)
			{
				nlog("ERROR: parsing logfile command: " << line_data);
				client_userinfo_bug.reset();
				continue;
			}
			else
			{
				log("INFO: ClientUserinfoChanged bug detected");
				cmd = "ClientUserinfoChanged";
				iss.clear();
				iss.str(client_userinfo_bug.params + line_data);
				log("INFO: params: " << client_userinfo_bug.params << line_data);
			}
		}

		client_userinfo_bug.reset();

		cmd += ":";

		str params;

		sgl(iss, params); // not all commands have params

		iss.clear();
		iss.str(params);

		if(cmd == "ClientUserinfoChanged:")
		{
			slot num;
			siz team;
			if(!(sgl(sgl(sgl(iss >> num, skip, '\\'), name, '\\'), skip, '\\') >> team))
				nlog("Error parsing ClientUserinfoChanged: "  << params);
			else if(num >= slot::max)
			{
				nlog("ERROR: Client num too high: " << num);
			}
			else
			{
				siz pos = line_data.find("\\id\\");
				if(pos == str::npos)
					client_userinfo_bug.set(params);
				else
				{
					str id = line_data.substr(pos + 4, 32);
					GUID guid(num);
					if(id.size() == 32)
						guid = GUID(id.substr(24));

					siz hc = 100;
					if((pos = line_data.find("\\hc\\")) == str::npos)
					{
						log("WARN: no handicap info found: " << line_data);
					}
					else
					{
						if(!(siss(line_data.substr(pos + 4)) >> hc))
							log("ERROR: Parsing handicap: " << line_data.substr(pos + 4));
					}

					clients[num] = guid;
					players[guid] = name;
					teams[guid] = team; // 1 = red, 2 = blue, 3 = spec
				}
			}
		}
		else if(cmd == "ClientConnect:")
		{
			slot num;
			if(!(iss >> num))
				nlog("Error parsing ClientConnect: "  << params);
			else
			{
				connected[siz(num)] = true;
			}
		}
		else if(cmd == "ClientBegin:") // 0:04 ClientBegin: 4
		{
			slot num;
			if(!(iss >> num))
				nlog("Error parsing ClientBegin: "  << params);
			else
			{
				connected[siz(num)] = true; // start trusting ClientUserinfoChanged
			}
		}
		else if(cmd == "ClientDisconnect:")
		{
			slot num;
			if(!(iss >> num))
				nlog("Error parsing ClientDisconnect: "  << params << ": " << line_data);
			else if(num >= slot::max)
			{
				nlog("ERROR: Client num too high: " << num << ": " << line_data);
			}
			else
			{
				connected[siz(num)] = false;
				// slot numbers are defunct, but keep GUIDs until ShutdownGame
				const GUID& guid = getClientGuid(num);

				// Sometimes you get 2 ClientDisconnect: events with
				// nothing created in between them. These should be ignored.
				if(guid == null_guid)
				{
					// partially connected slot num?
					clients.erase(num);
					continue;
				}

				getClientGuid(num).disconnect();

				teams[guid] = TEAM_U;
				clients.erase(num);
			}
		}
		else if(cmd == "InitGame:")
		{
			static str key, val;

			clients.clear();
			players.clear();
			teams.clear();
			svars.clear();

			iss.ignore(); // skip initial '\\'
			while(sgl(sgl(iss, key, '\\'), val, '\\'))
				svars[key] = val;

			mapname = svars["mapname"];
			mod_katina = svars["mod_katina"];
		}
	}

	return true;
}

bool Katina::start(const str& dir)
{
	log("Starting Katina:");
	config_dir = expand_env(dir);
	log("Setting config dir: " << dir);

	load_config(config_dir, "katina.conf", props);

	runmode = get("runmode", get("run.mode", "live"));
	name = get("katina.name" , "^1K^7at^3i^7na^7");

	bool live = (runmode == "live");
	bool rerun = (runmode == "rerun");
	bool backlog = (runmode == "backlog");

	if(!init_pki())
		return false;

	init_rcon();
	if(rerun || backlog)
		server.off();

	// everything the plugins need shuld be initialized before loading them

	// Get mod_katina version if available
	if(!rconset("mod_katina", mod_katina))
		if(!rconset("mod_katina", mod_katina))
			mod_katina.clear();

	now = get("runtime", std::time(0));
	std::time_t base_now = now; // rerun base time

	bug_var(now);

	load_plugins();

	std::ios::openmode mode = std::ios::in|std::ios::ate;

	if(rerun)
		mode = std::ios::in;

	log("Opening logfile (" << runmode << "): " << get("logfile"));

	std::ifstream ifs;

	const str_vec& logfiles = get_exp_vec("logfile");

	for(const str& logfile: logfiles)
	{
		done = false;
		ifs.clear();
		ifs.open(logfile, mode);

		if(!ifs.is_open())
		{
			if(!rerun)
			{
				log("FATAL: Logfile not found: " << get("logfile"));
				return false;
			}
			log("WARN: Logfile not found: " << get("logfile"));
			continue;
		}

		std::istream& is = ifs;
		std::ios::streampos gpos = is.tellg();

		line_number = 0; // log file line number
		if(!rerun)
		{
			// read back through log file to build up current player
			// info
			std::time_t rbt = std::time(0);
			log("Initializing data structures");
			if(!read_backlog(get_exp("logfile"), gpos))
				log("WARN: Unable to get initial player info");
			log("DONE: " << (std::time(0) - rbt) << " seconds");
		}

		if(backlog) // read backlog only
			break;

		client_userinfo_bug_t client_userinfo_bug;

		log("Processing:");

		//===================================================//
		//= MAIN LOOP									   =//
		//===================================================//

		// working variables
		char c;
		siz min, sec;
		siz prev_sec = siz(-1);
		str skip, name, cmd;
		siss iss;

		while(!done)
		{
			if(!std::getline(is, line_data) || is.eof())
			{
				if(rerun)
					done = true;
				thread_sleep_millis(100);
				is.clear();
				is.seekg(gpos);
				continue;
			}

			++line_number;
			if(do_log_lines)
				nlog("LINE: " << line_data);

			gpos = is.tellg();

			if(!active)
				continue;

			if(trim(line_data).empty())
				continue;

			iss.clear();
			iss.str(line_data);

			if(!(sgl(iss >> min >> c >> sec >> std::ws, cmd, ':') >> std::ws))
			{
				if(!client_userinfo_bug)
				{
					nlog("ERROR: parsing logfile command: " << line_data);
					continue;
				}
				log("WARN: possible ClientUserinfoChanged bug");
				if(line_data.find("\\id\\") == str::npos)
				{
					nlog("ERROR: parsing logfile command: " << line_data);
					client_userinfo_bug.reset();
					continue;
				}
				else
				{
					nlog("INFO: ClientUserinfoChanged bug detected");
					cmd = "ClientUserinfoChanged";
					iss.clear();
					iss.str(client_userinfo_bug.params + line_data);
					nlog("INFO: params: " << client_userinfo_bug.params << line_data);
				}
			}

			client_userinfo_bug.reset();

			if(!cmd.find("----"))
			{
				if(!min && !sec)
				{
					if(live || have("runtime"))
					{
						base_now = std::time(0);
						bug("=========================");
						bug("= BASE_TIME: " << base_now << " =");
						bug("=========================");
					}
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

			for(const evt_erase& e: erase_events)
			{
				plugin_lst_iter i = std::find(events[e.e].begin(), events[e.e].end(), e.p);
				if(i != events[e.e].end())
					events[e.e].erase(i);
			}
			erase_events.clear();

			// Send HEARTBEAT event to plugins
			if(sec != prev_sec)
			{
				prev_sec = sec; // only once every second (at most)

				siz time_in_secs = (min * 60) + sec;
				siz regularity;
				for(plugin_lst_iter i = events[HEARTBEAT].begin(); i != events[HEARTBEAT].end(); ++i)
					if((regularity = (*i)->get_regularity(time_in_secs)) && !(time_in_secs % regularity))
						(*i)->heartbeat(min, sec);
			}
			bool flagspeed = false; // speed carrying a flag

			if(cmd == "Exit:")
			{
				//bug(cmd << "(" << params << ")");
				if(events[EXIT].empty())
					continue;

				for(plugin_lst_iter i = events[EXIT].begin()
					; i != events[EXIT].end(); ++i)
					(*i)->exit(min, sec);
			}
			else if(cmd == "ShutdownGame:")
			{
				for(plugin_lst_iter i = events[SHUTDOWN_GAME].begin()
					; i != events[SHUTDOWN_GAME].end(); ++i)
					(*i)->shutdown_game(min, sec);
			}
			else if(cmd == "Warmup:")
			{
				if(events[WARMUP].empty())
					continue;

				for(plugin_lst_iter i = events[WARMUP].begin()
					; i != events[WARMUP].end(); ++i)
					(*i)->warmup(min, sec);
			}
			else if(cmd == "ClientUserinfoChanged:")
			{
				slot num;
				siz team;
				if(!(sgl(sgl(sgl(iss >> num, skip, '\\'), name, '\\'), skip, '\\') >> team))
					nlog("Error parsing ClientUserinfoChanged: "  << params);
				else if(num >= slot::max)
					nlog("ERROR: Client num too high: " << num);
				else
				{
					siz pos = line_data.find("\\id\\");
					if(pos == str::npos)
						client_userinfo_bug.set(params);
					else
					{
						str id = line_data.substr(pos + 4, 32);
						GUID guid(num);
						if(id.size() == 32)
							guid = GUID(id.substr(24));

						siz hc = 100;
						if((pos = line_data.find("\\hc\\")) == str::npos)
							nlog("WARN: no handicap info found: " << line_data);
						else
						{
							if(!(siss(line_data.substr(pos + 4)) >> hc))
								nlog("ERROR: Parsing handicap: " << line_data.substr(pos + 4));
						}

						siz teamBefore = teams[guid];

						clients[num] = guid;
						players[guid] = name;
						teams[guid] = team; // 1 = red, 2 = blue, 3 = spec

						for(plugin_lst_iter i = events[CLIENT_USERINFO_CHANGED].begin();
							i != events[CLIENT_USERINFO_CHANGED].end(); ++i)
								(*i)->client_userinfo_changed(min, sec, num, team, guid, name, hc);

						if(team != teamBefore && !guid.is_bot())
							for(plugin_lst_iter i = events[CLIENT_SWITCH_TEAM].begin();
								i != events[CLIENT_SWITCH_TEAM].end(); ++i)
									(*i)->client_switch_team(min, sec, num, teamBefore, team);
					}
				}
			}
			else if(cmd == "ClientConnect:")
			{
				slot num;
				if(!(iss >> num))
					nlog("Error parsing ClientConnect: "  << params);
				else
				{
					//if(!is_connected(num))
					//clients[num] = null_guid; // connecting
					connected[siz(num)] = true;
					for(plugin_lst_iter i = events[CLIENT_CONNECT].begin()
						; i != events[CLIENT_CONNECT].end(); ++i)
						(*i)->client_connect(min, sec, num);
				}
			}
			else if(cmd == "ClientConnectInfo:")
			{
				// ClientConnectInfo: 4 87597A67B5A4E3C79544A72B7B5DA741 81.101.111.32
				nlog("ClientConnectInfo: " << params);
				if(events[CLIENT_CONNECT_INFO].empty())
					continue;

				slot num;
				str guid;
				str ip;
				str skip; // rest of guid needs to be skipped before ip

				// 2 5E68E970866FC20242482AA396BBD43E 81.101.111.32
				if(!(iss >> num >> std::ws >> guid >> std::ws >> ip))
					nlog("Error parsing ClientConnectInfo: "  << params);
				else if(!is_connected(num))
				{
					// ignore this event until it occurs at a reliable place
					if(mod_katina == "0.1.1")
						nlog("ERROR: This event should NEVER occur in 0.1.1");
					if(mod_katina >= "0.1.2")
						nlog("ERROR: This event should NEVER occur after 0.1.2");
				}
				else
				{
					for(plugin_lst_iter i = events[CLIENT_CONNECT_INFO].begin()
						; i != events[CLIENT_CONNECT_INFO].end(); ++i)
						(*i)->client_connect_info(min, sec, num, GUID(guid), ip);
				}
			}
			else if(cmd == "ClientBegin:") // 0:04 ClientBegin: 4
			{
//				if(events[CLIENT_BEGIN].empty())
//					continue;

				slot num;
				if(!(iss >> num))
					nlog("Error parsing ClientBegin: "  << params);
				else
				{
					connected[siz(num)] = true; // start trusting ClientUserinfoChanged
					for(plugin_lst_iter i = events[CLIENT_BEGIN].begin()
						; i != events[CLIENT_BEGIN].end(); ++i)
						(*i)->client_begin(min, sec, num);
				}
			}
			else if(cmd == "ClientDisconnect:")
			{
				//bug(cmd << "(" << params << ")");

				slot num;
				if(!(iss >> num))
					std::cout << "Error parsing ClientDisconnect: "  << params << '\n';
				else if(num >= slot::max)
				{
					nlog("ERROR: Client num too high: " << num);
				}
				else
				{
					connected[siz(num)] = false;
					const GUID& guid = getClientGuid(num);

					// Sometimes you get 2 ClientDisconnect: events with
					// nothing created in between them. These should be ignored.
					if(guid == null_guid)
						continue;

					guid.disconnect();

					for(plugin_lst_iter i = events[CLIENT_DISCONNECT].begin()
						; i != events[CLIENT_DISCONNECT].end(); ++i)
						(*i)->client_disconnect(min, sec, num);

					siz teamBefore = teams[guid];
					teams[guid] = TEAM_U;

				   if(teamBefore != TEAM_U && !guid.is_bot())
						for(plugin_lst_iter i = events[CLIENT_SWITCH_TEAM].begin();
								i != events[CLIENT_SWITCH_TEAM].end(); ++i)
							(*i)->client_switch_team(min, sec, num, teamBefore, TEAM_U);

					clients.erase(num);
				}
			}
			else if(cmd == "Kill:")
			{
				if(events[KILL].empty())
					continue;

				slot num1, num2;
				siz weap;
				//   9:01 Kill: 1022 0 19: <world> killed Merman by MOD_FALLING
				// 1022 1 22: <world> killed Arachna by MOD_TRIGGER_HURT
				if(!(iss >> num1 >> num2 >> weap))
					nlog("Error parsing Kill: " << params);
				else
				{
					for(plugin_lst_iter i = events[KILL].begin()
						; i != events[KILL].end(); ++i)
						(*i)->kill(min, sec, num1, num2, weap);
				}
			}
			else if(cmd == "Push:") // mod_katina only
			{
				//bug(cmd << "(" << params << ")");
				if(events[PUSH].empty())
					continue;

				slot num1, num2;
				if(!(iss >> num1 >> num2))
					nlog("Error parsing Push:" << params);
				else
				{
					for(plugin_lst_iter i = events[PUSH].begin()
						; i != events[PUSH].end(); ++i)
						(*i)->push(min, sec, num1, num2);
				}
			}
			else if(cmd == "WeaponUsage:")
			{
				// Weapon Usage Update
				// WeaponUsage: <client#> <weapon#> <#shotsFired>
				slot num;
				siz weap, shots;

				if(iss >> num >> weap >> shots)
				{
					for(plugin_lst_iter i = events[WEAPON_USAGE].begin(); i != events[WEAPON_USAGE].end(); ++i)
						(*i)->weapon_usage(min, sec, num, weap, shots);
				}
				else
					nlog("Error parsing WeaponUsage: " << params);
			}
			else if(cmd == "MODDamage:")
			{
				// MOD (Means of Death = Damage Type) Damage Update
				// MODDamage: <client#> <mod#> <#hits> <damageDone> <#hitsRecv> <damageRecv> <weightedHits>
				slot num;
				siz mod, hits, dmg, hitsRecv, dmgRecv;
				float weightedHits;
				if(iss >> num >> mod >> hits >> dmg >> hitsRecv >> dmgRecv >> weightedHits)
				{
					for(plugin_lst_iter i = events[MOD_DAMAGE].begin(); i != events[MOD_DAMAGE].end(); ++i)
						(*i)->mod_damage(min, sec, num, mod, hits, dmg, hitsRecv, dmgRecv, weightedHits);
				}
				else
					nlog("Error parsing MODDamage: " << params);
			}
			else if(cmd == "PlayerStats:")
			{
				// Player Stats Update
				// PlayerStats: <client#>
				// 				<fragsFace> <fragsBack> <fraggedInFace> <fraggedInBack>
				// 				<spawnKillsDone> <spanwKillsRecv>
				// 				<pushesDone> <pushesRecv>
				// 				<healthPickedUp> <armorPickedUp>
				//				<holyShitFrags> <holyShitFragged>
				slot num;
				siz fragsFace, fragsBack, fraggedFace, fraggedBack, spawnKills, spawnKillsRecv;
				siz pushes, pushesRecv, health, armor, holyShitFrags, holyShitFragged;
				if(iss >> num >> fragsFace >> fragsBack >> fraggedFace >> fraggedBack >> spawnKills >> spawnKillsRecv
					   >> pushes >> pushesRecv >> health >> armor >> holyShitFrags >> holyShitFragged)
				{
					for(plugin_lst_iter i = events[PLAYER_STATS].begin(); i != events[PLAYER_STATS].end(); ++i)
					{
						(*i)->player_stats(min, sec, num,
							fragsFace, fragsBack, fraggedFace, fraggedBack,
							spawnKills, spawnKillsRecv, pushes, pushesRecv,
							health, armor, holyShitFrags, holyShitFragged);
					}
				}
				else
					nlog("Error parsing PlayerStats: " << params);
			}
			else if(cmd == "CTF:")
			{
				if(events[CTF].empty())
					continue;

				slot num;
				siz col, act;
				if(!(iss >> num >> col >> act) || col < 1 || col > 2)
					nlog("Error parsing CTF:" << params);
				else
				{
					for(plugin_lst_iter i = events[CTF].begin()
						; i != events[CTF].end(); ++i)
						(*i)->ctf(min, sec, num, col, act);
				}
			}
			else if(cmd == "red:") // BUG: red:(8  blue:6) [Katina.cpp] (662)
			{
				if(events[CTF_EXIT].empty())
					continue;

				siz r = 0;
				siz b = 0;
				str skip;
				if(!(sgl(iss >> r >> std::ws, skip, ':') >> b))
					nlog("Error parsing CTF_EXIT:" << params);
				else
				{
					for(plugin_lst_iter i = events[CTF_EXIT].begin()
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
				slot num;
				str name;

				if(!sgl(iss >> score >> skip >> ping >> skip >> num >> std::ws, name))
					nlog("Error parsing SCORE_EXIT:" << params);
				else
				{
					for(plugin_lst_iter i = events[SCORE_EXIT].begin()
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

				slot num;
				siz dist, time;
				if(!(iss >> num >> dist >> time))
					nlog("Error parsing Speed:" << params);
				else
				{
					for(plugin_lst_iter i = events[SPEED].begin()
						; i != events[SPEED].end(); ++i)
						(*i)->speed(min, sec, num, dist, time, flagspeed);
				}
			}
			else if(cmd == "Award:")
			{
				//bug(cmd << "(" << params << ")");
				if(events[AWARD].empty())
					continue;

				slot num;
				siz awd;
				if(!(iss >> num >> awd))
					nlog("Error parsing Award:" << params);
				else
				{
					for(plugin_lst_iter i = events[AWARD].begin()
						; i != events[AWARD].end(); ++i)
						(*i)->award(min, sec, num, awd);
				}
			}
			else if(cmd == "InitGame:")
			{
				bug("== INIT GAME ==");
				bug_var(rerun);
				bug_var(have("runtime"));

				static str key, val;

				clients.clear();
				players.clear();
				teams.clear();
				svars.clear();

				iss.ignore(); // skip initial '\\'
				while(sgl(sgl(iss, key, '\\'), val, '\\'))
					svars[key] = val;

				mapname = svars["mapname"];
				mod_katina = svars["mod_katina"];

				bug_var(svars.size());
				bug_var(svars["g_timestamp"]);

				if(rerun && !have("runtime"))
				{
					str skip;
					siz Y, M, D, h, m, s;
					char c;
					siss iss(svars["g_timestamp"]);
					// g_timestamp 2013-05-24 09:34:32
					if(!(iss >> Y >> c >> M >> c >> D >> c >> h >> c >> m >> c >> s))
						nlog("ERROR: parsing g_timestamp: " << svars["g_timestamp"]);
					else
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
						now = base_now = std::mktime(&t);
						nlog("RERUN TIMESTAMP: " << base_now);
					}
				}

				str msg = this->name + " ^3Stats System v^7" + version + (tag.size()?"^3-^7":"") + tag;
				server.cp(msg);

				nlog("MAP NAME: " << mapname);

				if(events[INIT_GAME].empty())
					continue;

				for(plugin_lst_iter i = events[INIT_GAME].begin()
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

				slot num;
				str type;
				str info;

				if(!sgl(iss >> num >> std::ws >> type >> std::ws, info, ':'))
					nlog("Error parsing Callvote: "  << params);
				else
				{
					for(plugin_lst_iter i = events[LOG_CALLVOTE].begin()
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

				if(extract_name_from_text(line_data, guid, text))
					for(plugin_lst_iter i = events[SAYTEAM].begin()
						; i != events[SAYTEAM].end(); ++i)
						(*i)->sayteam(min, sec, guid, text);
			}
			else if(cmd == "say:")
			{
				if(events[SAY].empty())
					continue;

				str text;
				GUID guid;

				if(extract_name_from_text(line_data, guid, text))
				{
					if(!text.find("!katina"))
						builtin_command(guid, text);
					else
						for(plugin_lst_iter i = events[SAY].begin()
							; i != events[SAY].end(); ++i)
							(*i)->say(min, sec, guid, text);
				}
			}
			else if(cmd == "chat:")
			{
				if(events[CHAT].empty())
					continue;

				for(plugin_lst_iter i = events[CHAT].begin()
					; i != events[CHAT].end(); ++i)
					(*i)->chat(min, sec, params);
			}
			else
			{
				if(events[UNKNOWN].empty())
					continue;

				//bug("UNKNOWN: " << cmd << "(" << params << ")");

				for(plugin_lst_iter i = events[UNKNOWN].begin()
					; i != events[UNKNOWN].end(); ++i)
					(*i)->unknown(min, sec, cmd, params);
			}
			//pthread_mutex_unlock(&cvarevts_mtx);
		}

		// only process first logfile when
		// running live
		if(!rerun)
			break;
	}

	log("TERMINATE: all good");

	return true;
}

} // katina
