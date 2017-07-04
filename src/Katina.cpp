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

#include <katina/radp.h>

#include <ctime>
#include <string>
#include <cstring>
#include <bitset>
#include <cassert>

#define ID "katina"

//#define QUOTE(x) #x
//#define REV QUOTE(REVISION)
//#define REV QUOTE(REVISION)

namespace katina {

using namespace katina::log;
using namespace katina::net;
using namespace katina::time;
using namespace katina::types;
using namespace katina::utils;
using namespace katina::string;

using namespace sookee::radp;

static const str version = "0.3";
static const str tag = QUOTE(DEV);
static const str revision = QUOTE(REVISION);

siz Katina::getTeam(slot num) const
{
	return clients[siz(num)].team;
}

//siz Katina::getTeam(const GUID& guid) const
//{
//	guid_siz_map_citer teamsIt = teams.find(guid);
//	if(teamsIt == teams.end())
//		return TEAM_U;
//
//	return teamsIt->second;
//}

//str Katina::getPlayerName(slot num) const
//{
//	return clients[siz(num)].name;
//}

str Katina::getPlayerName(const GUID& guid) const
{
	guid_str_map_citer p;
	if((p = players.find(guid)) == players.cend())
		return "";

	return p->second;
}
//
slot Katina::getClientSlot(const GUID& guid) const
{
	for(slot i(0); i < clients.size(); ++i)
		if(clients[i].guid == guid)
			return slot(i);
	return slot::bad;
}

const GUID&  Katina::getClientGuid(slot num) const
{
	return clients[siz(num)].guid;
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

slot Katina::slot_from_name(const str& name)
{
	for(uns i = 0; i < clients.size(); ++i)
	{
		if(!clients[i].live)
			continue;
		if(clients[i].name == name)
			return slot(i);
	}
	return slot::bad;
}

slot Katina::extract_num(const str& line)
{
	slot num;
	for(siz pos = 0; (pos = line.find(": ", pos)) != str::npos; pos += 2)
	{
		if((num = slot_from_name(line.substr(0, pos))) == slot::bad)
			continue;

		if(!clients[siz(num)].live)
			continue;

		return num;
	}
	return slot::bad;
}

//GUID Katina::extract_name(const str& line)//, str& text)
//{
//	slot num;
//	for(siz pos = 0; (pos = line.find(": ", pos)) != str::npos; pos += 2)
//	{
//		if((num = slot_from_name(line.substr(0, pos))) == slot::bad)
//			continue;
//
//		if(!clients.count(num))
//			continue;
//
//		return clients[num];
//	}
//	return null_guid;
//}

bool Katina::load_plugin(const str& file, const unsigned priority)
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

	// ensure only one of each type of plugin
	if(plugins.count(plugin->get_id()))
	{
		log("PLUGIN LOAD: duplicate plugin id: " << plugin->get_id());
		if(dlclose(dl))
			log("PLUGIN LOAD: plugin failed to unload: " << dlerror());
		return false;
	}

	plugin->dl = dl;
	plugin->priority = priority;

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
	bug_func();
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

//struct node
//{
//	using ptr_vec = std::vector<node*>;
//	str id;
//	ptr_vec to;
//	node(const str& id = ""): id(id) {}
//};
//
//using node_vec = std::vector<node>;
//
//void dep_resolve(node* n, node::ptr_vec& seen, node::ptr_vec& resolved)
//{
//	bug("dep_resolve: " << n->id);
//	seen.push_back(n);
//	for(auto np: n->to)
//	{
//		if(std::find(resolved.begin(), resolved.end(), np) != resolved.end())
//		{
//			bug("   skipping: " << np->id);
//			continue;
//		}
//		if(std::find(seen.begin(), seen.end(), np) == seen.end())
//			dep_resolve(np, seen, resolved);
//		else
//		{
//			log("X: plugin " << n->id << " has a circular dependency with " << np->id);
//			continue;
//		}
//	}
//	bug("     adding: " << n->id);
//	resolved.push_back(n);
//}

class Resolver
{
	str_vec seen;
	str_vec resolved;
	std::map<str, str_vec> deps;

	void resolve(str const& d)
	{
		bug("dep_resolve: " << d);
		seen.push_back(d);
		for(auto const& nd: deps[d])
		{
			if(std::find(resolved.begin(), resolved.end(), nd) != resolved.end())
				continue;
			else if(std::find(seen.begin(), seen.end(), nd) == seen.end())
				resolve(nd);
			else
			{
				log("E: plugin " << d << " has a circular dependency with " << nd);
				continue;
			}
		}
		bug("     adding: " << d);
		resolved.push_back(d);
	}

public:

	// clear internal state without
	// reducing memory allocation
	void clear()
	{
		seen.clear();
		resolved.clear();
		deps.clear();
	}

	// clear and reduce memory consumption
	// to minimum
	void clean(unsigned reserve_guess = 20)
	{
		str_vec().swap(seen);
		seen.reserve(reserve_guess);
		str_vec().swap(resolved);
		resolved.reserve(reserve_guess);
		std::map<str, str_vec>().swap(deps);
	}

	void add(str const& a)
	{
		deps[a];
		seen.reserve(deps.size());
	}

	void add(str const& a, str const& b)
	{
		deps[a].push_back(b);
		seen.reserve(deps.size());
	}

	str_vec const& resolve()
	{
		for(auto const& d: deps)
			if(std::find(resolved.begin(), resolved.end(), d.first) == resolved.end())
				resolve(d.first);
		return resolved;
	}
};

void Katina::load_plugins()
{
	bug_func();
	log("Loading plugins:");
//	str_vec pluginfiles = get_exp_vec("plugin");
	//	for(siz i = 0; i < pluginfiles.size(); ++i)
	//		load_plugin(pluginfiles[i]);

	unsigned priority = 0;
	for(auto& file: get_exp_vec("plugin"))
		if(load_plugin(file, priority))
			++priority;

	// resolve dependencies
	bug("== Resolving dependencies: =================================");
	str_vec deps;

	for(auto& p: plugins)
	{
		bug("adding plugin id: " << p.first);
		deps.emplace_back(p.first);
	}

	bug("");

	Resolver resolver;

	for(auto const& d: deps)
	{
		bug("plugin: " << d);
		resolver.add(d);
		for(auto const& id: plugins[d]->get_parent_plugin_ids())
		{
			bool done = false;
			for(auto const& dd: deps)
			{
				if(dd != id)
					continue;
				resolver.add(d, id);
				bug("   dep: " << id);
				done = true;
				break;
			}
			if(!done)
			{
				log("W: dependency '" << id << "' for " << d << " not found");
			}
		}
	}

//	bug("== Resolving dependencies: =================================");
//	node_vec nodes;
//
//	for(auto& p: plugins)
//	{
//		bug("adding plugin id: " << p.first);
//		nodes.emplace_back(p.first);
//	}
//
//	bug("");
//
//	for(auto& n: nodes)
//	{
//		bug("plugin: " << n.id << " [" << &n << "]");
//		for(auto const& id: plugins[n.id]->get_parent_plugin_ids())
//		{
//			bool done = false;
//			for(auto& nn: nodes)
//			{
//				if(nn.id != id)
//					continue;
//				n.to.push_back(&nn);
//				bug("   dep: " << nn.id << " [" << &nn << "]");
//				done = true;
//				break;
//			}
//			if(!done)
//			{
//				log("W: dependency '" << id << "' for " << n.id << " not found");
//			}
//		}
//	}
//
//	node::ptr_vec seen;
//	node::ptr_vec resolved;
//
//	for(auto& n: nodes)
//		if(std::find(resolved.begin(), resolved.end(), &n) == resolved.end())
//			dep_resolve(&n, seen, resolved);
//
	// resolved should now contain the priority order of the plugins

	bug("");

	priority = 0;
//	for(auto& n: resolved)
	for(auto const& id: resolver.resolve())
	{
		bug("resolved: " << id << "[" << priority << "]");
		plugins[id]->priority = priority++;
	}

	// TODO: needs testing!!
	bug("============================================================");

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

bool Katina::has_plugin(const str& id, const str& version)
{
	plugin_map_iter i = plugins.find(id);

	if(i == plugins.end())
		return false;

	if(i->second->get_version() < version)
		return false;

	return true;
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
	return chat_to(clients[num].name, text);
}

//bool Katina::chat_to(const GUID& guid, const str& text)
//{
//	return chat_to(getPlayerName(guid), text);
//}

bool Katina::chat_to(const str& name, const str& text)
{
	return server.s_chat(name + "^2 " + text);
}

bool Katina::is_admin(const GUID& guid)
{
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
	{
		bug_var(line);
		if(trim(line) != "[admin]")
			continue;

		str keep = line;
		if(!(sgl(sgl(ifs, line), line))
		|| !(siss(line) >> line >> line >> line))
		{
			log("ERROR: parsing [admin]: " << keep);
			continue;
		}

		if(trim(line).size() == 32 && guid == GUID(line))
			return true;
	}
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
//		bug(line << " " << no);

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
			log("W: error parsing key entry: " << key);
			continue;
		}

		log("Reading public key for: [" << id << "] " << file);
		if(pki.add_client_key_file(id, config_dir + "/" + file))
			log("I: client key loaded  : [" << id << "] " << file);
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

void Katina::builtin_command(slot num, const str& text)
{
	bug_func();
	bug_var(num);
	bug_var(text);

	if(trim_copy(text).empty())
	{
		server.msg_to(num, name + "^7: " + revision);
		return;
	}

	if(!is_admin(clients[num].guid))
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
				for(siz i = 0; i< clients.size(); ++i)
				{
					oss << "{";
					oss << (clients[i].live?"C":"D");
					oss << (clients[i].bot?"B":"H");
					oss <<  ":" + str(i<10?" ":"") + str(slot(i)) + "," + str(clients[i].guid);
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
				TYPEDEF_MMAP(siz, siz, siz_siz_mmap);

				siz_siz_mmap sorted;

				for(siz i = 0; i< clients.size(); ++i)
					if(clients[i].live)
						sorted.insert(std::make_pair(clients[i].team, i));
//						sorted.emplace(clients[i].team, i);

				server.msg_to(num, "teams: " + std::to_string(sorted.size()));

				oss.str("");
				for(const auto& p: sorted)
				{
					oss << "{";
					oss << (clients[p.second].live?"C":"D");
					oss << (clients[p.second].bot?"B":"H");
					oss << ":" + std::to_string(p.first) + "," + clients[p.second].name;
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
				using std::begin;
				using std::end;
				server.msg_to(num, "players: " + std::to_string(players.size()));
				oss.str("");
				for(auto&& p: players)
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
				using std::begin;
				using std::end;
				siz c = std::count_if(begin(clients), end(clients), [](const client_t& c){return c.live;});
				server.msg_to(num, "connected: " + std::to_string(c));
				oss.str("");
				c = 0;
				for(siz i = 0; i < clients.size(); ++i)
				{
					auto& client = clients[i];

					if(!client.live)
						continue;

					oss << "{" << i << ": " << client.guid << " " << client.name << "}";

					if(oss.str().size() > WIDTH)
					{
						server.msg_to(num, oss.str());
						oss.str("");
					}
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
					log("set cvar: " << p << "::" << var << ": " << val << " [" << clients[num].name << "]");
				}
			}
		}
	}
}

//bool Katina::initial_player_info()
//{
//	//bug_func();
//	// ����	  !listplayers: 9 players connected:
//	//  2 S 5  Server Operator (*7B5DA741)   SooKee|AFK (a.k.a. SooKee)
//	//  3 S 0   Unknown Player (*198025FD)   Kiki
//	//  4 B 0   Unknown Player (*2671CC00)   Bruttler
//	//  5 S 0   Unknown Player (*55EA82F1)   *M*^Mist1|AFK
//	//  6 S 0   Unknown Player (*E2128F20)   Michel
//	//  7 S 0   Unknown Player (*3DBE95A7)   Bloody [Boy]
//	//  8 S 0   Unknown Player (*6122B476)   C@tch3r
//	// 10 R 0   Unknown Player (*45B12012)   XZKTR
//	// 11 S 5  Server Operator (*F587F42F)   a Cold Slug (a.k.a. Ximxim)
//
//	str reply;
//	if(!server.command("!listplayers", reply))
//		if(!server.command("!listplayers", reply))
//			return false;
//
//	if(reply.find("!listplayers:") == str::npos)
//	{
//		log("ERROR: bad reply from !listplayers: " << reply);
//		return false;
//	}
//
//	slot num;
//	char team; // 1 = red, 2 = blue, 3 = spec
//
//	siss iss(reply);
//	str line, skip, guid, name;
//
//	sgl(iss, line); // ignore first line
//
//	while(sgl(iss, line))
//	{
//		//bug_var(line);
//		siss iss(line);
//		if(!sgl(sgl(iss >> num >> std::ws >> team, skip, '*'), guid, ')'))
//		{
//			log("ERROR: parsing !listplayers: " << line);
//			continue;
//		}
//
//		if(num >= slot::max)
//		{
//			log("ERROR: Bad client num: " << num);
//			continue;
//		}
//
//		bug_var(num);
//		bug_var(team);
//		bug_var(guid);
//
//		if(guid.size() != 8)
//		{
//			bug("BOT FOUND: " << num);
//			clients[num] = GUID(num); // bot constructor
//			if(!clients[num].is_bot())
//				log("ERROR: not set to bot");
//		}
//		else
//		{
//			bug("HUMAN FOUND: " << num);
//			clients[num] = GUID(guid);
//			if(clients[num].is_bot())
//				log("ERROR: set to bot");
//		}
//
//		//bug("Adding: " << num << " to team " << team);
//		bug_var(getClientGuid(num));
//		teams[getClientGuid(num)] = (team == 'R' ? 1 : (team == 'B' ? 2 : 3));
//	}
//
//	if(!server.command("status", reply))
//		if(!server.command("status", reply))
//			{ log("ERROR: No status"); return false; }
//
//
//	if(reply.find("map:") == str::npos)
//	{
//		log("ERROR: bad reply from status: " << reply);
//		return false;
//	}
//
//	// ����	  map: am_thornish
//	// num score ping name			lastmsg address			   qport rate
//	// --- ----- ---- --------------- ------- --------------------- ----- -----
//	//   2	 0   26 ^1S^2oo^3K^5ee^4|^7AFK^7	 100 81.101.111.32		 61881 25000
//	//   3	46  138 Silent^7			 50 96.241.187.112		  814  2500
//	//   4	32   98 ^2|<^8MAD^1Kitty Too^7	   0 178.66.13.135		 14639 25000
//
//	iss.clear();
//	iss.str(reply);
//
//	bug_var(reply);
//
//	const str term = "^7";
//
//	while(sgl(iss, line) && line.find("---")) {}
////	bug("parse proper");
//	while(sgl(iss, line))
//	{
//		if(trim(line).empty())
//			continue;
////		bug_var(line);
//		siss iss(line);
//		if(!sgl(iss >> num >> skip >> skip >> std::ws, line))
//		{
//			log("ERROR: parsing status: " << line);
//			continue;
//		}
//		// ^2|<^8MAD^1Kitty Too^7	   0 178.66.13.135		 14639 25000
//		str_iter f = std::find_end(line.begin(), line.end(), term.begin(), term.end());
//		if(f == line.end())
//		{
//			log("ERROR: parsing status name: " << line);
//			continue;
//		}
//		players[clients[num]].assign(line.begin(), f).append(term);
//		bug_var(num);
//		bug_var(players[clients[num]]);
//
//		iss.clear();
//		iss.str(str(f, line.end()));
//
//		str ip;
//		if(iss >> skip >> std::ws >> skip >> std::ws >> ip)
//			bug("IP ADDRESS: " << ip);
//
//		// TODO: collect IPs in Katina (here too)
//		//bug_var(players[clients[num]]);
//	}
//
//	return true;
//}

// TODO: remove color codes
str sanitized(const str& name)
{
	return name;
}

bool Katina::parse_slot_guid_name(const str& slot_guid_name, slot& num)
{
	// 12 | A0B65FD9 | wibble

	if(slot_guid_name.size() > 2 && slot_guid_name.size() < 8)
	{
		for(siz i = 0; i < clients.size(); ++i)
		{
			auto& client = clients[i];

			if(!client.live)
				continue;

			// try GUID startswith
			if(!str(client.guid).find(upper_copy((slot_guid_name))))
				return (num = slot(i)) == i;
			// try name submatch
			if(sanitized(client.name).find(lower_copy(slot_guid_name)) != str::npos)
				return (num = slot(i)) == i;
			// try exact name match
			if(sanitized(client.name) == lower_copy(slot_guid_name))
				return (num = slot(i)) == i;
		}
	}

	if(slot_guid_name.size() < 3) // try slot match
		if(siss(slot_guid_name) >> num && check_slot(num))
			return num == num;

	return false;
}

// Sometimes the ClientUserinfoChanged message is split over
// two lines. This is a fudge to compensate for that bug
struct client_userinfo_bug_t
{
	siz min;
	siz sec;
	str params;

	void set(siz min, siz sec, const str& params)
	{
		this->min = min;
		this->sec = sec;
		this->params = params;
	}
	void reset() { params.clear(); }
	operator bool() { return !params.empty(); }
};

//bool Katina::read_backlog(const str& logname, std::ios::streampos pos)
//{
//	bug_func();
//	nlog("pos: " << pos);
//
//	sifs ifs(logname);
//
//	client_userinfo_bug_t client_userinfo_bug;
//
//	siss iss;
//	siz min, sec;
//	char c;
//	str cmd;
//	str skip;
//	str name;
//
//	line_number = 0;
//	while(ifs.getline(line_data, sizeof(line_data)))
//	{
//		if(ifs.tellg() >= pos)
//			return true;
//
//		++line_number; // current line number
//		if(do_log_lines)
//			nlog("LINE: " << line_data);
//
//		str line_data_s(line_data);
//
//		if(trim(line_data_s).empty())
//			continue;
//
//		iss.clear();
//		iss.str(line_data_s);
//
//		if(!(sgl(iss >> min >> c >> sec >> std::ws, cmd, ':') >> std::ws))
//		{
//			if(!client_userinfo_bug)
//			{
//				nlog("ERROR: parsing logfile command: " << line_data_s);
//				continue;
//			}
//			log("WARN: possible ClientUserinfoChanged bug");
//			if(line_data_s.find("\\id\\") == str::npos)
//			{
//				nlog("ERROR: parsing logfile command: " << line_data_s);
//				client_userinfo_bug.reset();
//				continue;
//			}
//			else
//			{
//				log("INFO: ClientUserinfoChanged bug detected");
//				cmd = "ClientUserinfoChanged";
//				iss.clear();
//				iss.str(client_userinfo_bug.params + line_data_s);
//				log("INFO: params: " << client_userinfo_bug.params << line_data_s);
//			}
//		}
//
//		client_userinfo_bug.reset();
//
//		cmd += ":";
//
//		str params;
//
//		sgl(iss, params); // not all commands have params
//
//		iss.clear();
//		iss.str(params);
//
//		if(cmd == "ClientUserinfoChanged:")
//		{
//			slot num;
//			siz team;
//			if(!(sgl(sgl(sgl(iss >> num, skip, '\\'), name, '\\'), skip, '\\') >> team))
//				nlog("Error parsing ClientUserinfoChanged: "  << params);
//			else if(trim_copy(name).empty())
//				nlog("ERROR: name is empty: " << params);
//			else if(num >= slot::max)
//				nlog("ERROR: Client num too high: " << num);
//			else
//			{
//				siz pos = line_data_s.find("\\id\\");
//				if(pos == str::npos)
//					client_userinfo_bug.set(min, sec, params);
//				else
//				{
//					str id = line_data_s.substr(pos + 4, 32);
//					GUID guid(num);
//					if(id.size() == 32)
//						guid = GUID(id.substr(24));
//
//					siz hc = 100;
//					if((pos = line_data_s.find("\\hc\\")) == str::npos)
//					{
//						log("WARN: no handicap info found: " << line_data_s);
//					}
//					else
//					{
//						if(!(siss(line_data_s.substr(pos + 4)) >> hc))
//							log("ERROR: Parsing handicap: " << line_data_s.substr(pos + 4));
//					}
//
//					clients[num].guid = guid;
//					clients[num].team = team; // 1 = red, 2 = blue, 3 = spec
//					clients[num].name = name;
//					players[guid] = name;
////					teams[guid] = team; // 1 = red, 2 = blue, 3 = spec
//				}
//			}
//		}
//		else if(cmd == "ClientConnect:")
//		{
//			slot num;
//			if(!(iss >> num))
//				nlog("Error parsing ClientConnect: "  << params);
//			else
//			{
//				if(num < slot::max)
//					clients[num].live = true;
//			}
//		}
//		else if(cmd == "ClientBegin:") // 0:04 ClientBegin: 4
//		{
//			slot num;
//			if(!(iss >> num))
//				nlog("Error parsing ClientBegin: "  << params);
//			else
//			{
//				if(num < slot::max)
//					clients[num].live = true;
//			}
//		}
//		else if(cmd == "ClientDisconnect:")
//		{
//			slot num;
//			if(!(iss >> num))
//				nlog("Error parsing ClientDisconnect: "  << params << ": " << line_data_s);
//			else if(num >= slot::max)
//			{
//				nlog("ERROR: Client num too high: " << num << ": " << line_data_s);
//			}
//			else
//			{
////				connected[siz(num)] = false;
//				// slot numbers are defunct, but keep GUIDs until ShutdownGame
//				clients[num].live = false;
//			}
//		}
//		else if(cmd == "InitGame:")
//		{
//			static str key, val;
//
//			for(auto&& client: clients)
//				client = client_t();
////			clients.clear();
////			players.clear();
////			teams.clear();
//			svars.clear();
//
//			iss.ignore(); // skip initial '\\'
//			while(sgl(sgl(iss, key, '\\'), val, '\\'))
//				svars[key] = val;
//
//			mapname = svars["mapname"];
//			timestamp = svars["g_timestamp"];
//			mod_katina = svars["mod_katina"];
//		}
//	}
//
//	return true;
//}

struct running_info
{
	uns_vec retry_counts;
	uns retry_idx = 0;
	uns retry_count = 0;
	uns retry_ave = 0;
	uns prev_retry_ave = 0;
	uns retry_sum = 0;
	uns retry_min = 10;
	uns retry_max = 1000;
	uns retry_millis = 0;
	uns prev_retry_millis = 0;

	running_info(uns size = 30): retry_counts(size) {}
	void add_val(uns v)
	{
		retry_sum -= retry_counts[retry_idx];
		retry_counts[retry_idx] = retry_count;
		retry_sum += retry_counts[retry_idx];
		retry_ave = retry_sum / retry_counts.size();

		++retry_idx;
		if(retry_idx == retry_counts.size())
			retry_idx = 0;

		retry_count = 0; // reset
	}
};

bool Katina::start(const str& dir)
{
	//on_scoper on();
	log("Starting Katina:");

//	log(std::hex << siz(line_data));
//	log(std::hex << siz(chk_Kill));
//	log(std::hex << siz(param_Kill));
//	log(std::hex << siz(cmd));
//
//	return false;

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

	load_plugins();

	std::ios::openmode mode = std::ios::in | std::ios::ate;

//	if(rerun)
//		mode = std::ios::in;

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

		const std::ios::streampos rerun_end = ifs.tellg();

		if(rerun)
			ifs.seekg(0);

		std::istream& is = ifs;
		std::ios::streampos gpos = is.tellg();

		line_number = 0; // log file line number
		std::time_t rbt = std::time(0);
		bool do_read_backlog = false;
		if(!rerun)
		{
			// read back through log file to build up current player
			// info
//			std::time_t rbt = std::time(0);
			log("Initializing data structures");
//			if(!read_backlog(get_exp("logfile"), gpos))
//				log("WARN: Unable to get initial player info");
//			log("DONE: " << (std::time(0) - rbt) << " seconds");
			do_read_backlog = true;
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
		int num, num1, num2;
		siz prev_sec = siz(-1);
		str skip, name, guid;
		siz r, b;

		// store slot number from "client:" message (mod_katina >= 0.2.2)
		slot client = slot::bad;

		rad iss;
		//rad params;

		running_info rinfo;

		while(!done)
		{
//			off_scoper off;
			if(!is.getline(line_data, sizeof(line_data)) || is.eof())
			{
				if(do_read_backlog)
					log("I: READ BACKLOG DONE: " << (std::time(0) - rbt) << " seconds");
				do_read_backlog = false;

				++rinfo.retry_count;
				if(rerun)
					done = true;

				if(!get("debug.running.info", false))
					std::this_thread::sleep_for(milliseconds(100));
				else
				{
					rinfo.retry_millis = rinfo.retry_min
						+ (((rinfo.retry_max - rinfo.retry_min) / rinfo.retry_max) * rinfo.retry_ave);
					if(rinfo.retry_millis > rinfo.retry_max)
						rinfo.retry_millis = rinfo.retry_max;
					if(rinfo.retry_millis < rinfo.retry_min)
						rinfo.retry_millis = rinfo.retry_min;

					if(rinfo.prev_retry_millis != rinfo.retry_millis)
						bug_var(rinfo.retry_millis);

					rinfo.prev_retry_millis = rinfo.retry_millis;

					std::this_thread::sleep_for(milliseconds(rinfo.retry_millis));
				}


				is.clear();
				is.seekg(gpos);
				continue;
			}
//			if(retry_count)
//			bug_var(retry_count);

			if(get("debug.running.info", false))
			{
				rinfo.add_val(rinfo.retry_count);
				if(rinfo.retry_ave != rinfo.prev_retry_ave)
					bug_var(rinfo.retry_ave);
				rinfo.prev_retry_ave = rinfo.retry_ave;
			}

			++line_number;
			if(do_log_lines)
				nlog("LINE: " << line_data);

			gpos = is.tellg();

			siz rerun_pc = 0;
			siz prev_rerun_pc = siz(-1);
			if(rerun && prev_rerun_pc != (rerun_pc = ((gpos * 100) / rerun_end)))
			{
				std::cerr << "\rProgress: " << rerun_pc << "%" << std::flush;
				prev_rerun_pc = rerun_pc;
			}

			if(!active)
				continue;

			if(!*ps(line_data))
				continue;

			if(*(iss = pz(pc(psz(line_data, min), c), sec)) && c != ':' && *iss != ' ')
			{
				if(!client_userinfo_bug)
				{
					nlog("ERROR: parsing logfile command: " << line_data);
					continue;
				}
				log("WARN: possible ClientUserinfoChanged bug");
				if(fnd(line_data, "\\id\\"))
				{
					nlog("ERROR: parsing logfile command: " << line_data);
					client_userinfo_bug.reset();
					continue;
				}
				else
				{
					nlog("INFO: ClientUserinfoChanged bug detected");
					str broken = line_data; // second half of ClientUserinfoChanged: params
					*line_data = 0;
					strcat(line_data, client_userinfo_bug.params.c_str());
					strcat(line_data, broken.c_str());
					nlog("INFO: line_data: " << line_data);
				}
			}

			client_userinfo_bug.reset();

			iss = pc(iss, c);

			if(c != ' ')
			{
				nlog("E: PARSE ERROR: expected c = ' ', found: " << c << " [0x" << std::hex << int(c) << "]");
				nlog("I: line_data: " << line_data);
				continue;
			}

			if(*iss == '-')
			{
				if(!min && !sec)
				{
					if(live || have("runtime"))
					{
						base_now = std::time(0);
						bug("=========================");
						log("= BASE_TIME: " << base_now << " =");
						bug("=========================");
					}
				}
				continue;
			}

			now = base_now + (min * 60) + sec;

			for(const auto& e: erase_events)
			{
				auto i = std::find(events[e.e].begin(), events[e.e].end(), e.p);
				if(i != events[e.e].end())
					events[e.e].erase(i);
			}
			erase_events.clear();

			// Send HEARTBEAT event to plugins
			if(!do_read_backlog && sec != prev_sec)
			{
				prev_sec = sec; // only once every second (at most)

				siz time_in_secs = (min * 60) + sec;
				siz regularity;
				for(auto plugin: events[KE_HEARTBEAT])
					if((regularity = plugin->get_regularity(time_in_secs)) && !(time_in_secs % regularity))
						plugin->heartbeat(min, sec);
			}

			bool flagspeed = false; // speed carrying a flag

			// 2015-08-14 14:18:15: PARSE ERROR: expected c = ' ', found: P [0x50]
			// 2015-08-14 14:18:15: PARSE ERROR: expected c = ' ', found: A [0x41]
			// 2015-08-14 14:19:22: PARSE ERROR: expected c = ' ', found: K [0x4b]
			// 2015-08-14 14:19:22: PARSE ERROR: expected c = ' ', found: P [0x50]
			// 2015-08-14 14:20:43: PARSE ERROR: expected c = ' ', found: K [0x4b]
			// 2015-08-14 14:20:43: PARSE ERROR: expected c = ' ', found: P [0x50]
			// 2015-08-14 14:21:21: PARSE ERROR: expected c = ' ', found: K [0x4b]
			// 2015-08-14 14:21:21: PARSE ERROR: expected c = ' ', found: P [0x50]
			// 2015-08-14 14:22:04: PARSE ERROR: expected c = ' ', found: S [0x53]
			// 2015-08-14 14:22:04: PARSE ERROR: expected c = ' ', found: - [0x2d]

			switch(*cmd)
			{
			case 'K':
				// Possible events sorted by frequency
				// most frequent first
				// Kill:
				// KatinaFlags:

				if((*chk_Kill) == ':')
				{
					if(events[KE_KILL].empty())
						break;

					siz weap;

					if(*rsp(param_Kill, num1, num2, weap) != ':')
						nlog("ERROR: parsing: " << cmd);
					else if(!do_read_backlog)
					{
						for(auto plugin: events[KE_KILL])
							plugin->kill(min, sec, slot(num1), slot(num2), weap);
					}
				}
				else if((*chk_KatinaFlags) == ':')
				{
					if(*rsp(param_KatinaFlags, mod_katina_flags))
						nlog("ERROR: parsing: " << cmd);
					else
					{
						log("setting mod_katina_flags: " << std::bitset<KATINA_FLAG_BITS>(mod_katina_flags));
					}
				}
			break;

			case 'A':
				// Possible events sorted by frequency
				// most frequent first
				// Award:

				if(events[KE_AWARD].empty())
					break;

				siz awd;

				if(*rsp(param_Award, num, awd) != ':')
					nlog("Error parsing Award: " << param_Award);
				else if(!do_read_backlog)
				{
					for(auto plugin: events[KE_AWARD])
						plugin->award(min, sec, slot(num), awd);
				}
			break;

			case 'C':
				// Possible events
				// CTF:
				// Callvote:
				// Challenge:
				// ClientBegin:
				// ClientConnect:
				// ClientDisconnect:
				// ClientConnectInfo:
				// ClientUserinfoChanged:

				if((*chk_CTF) == ':')
				{
					if(events[KE_CTF].empty())
						continue;

					siz col, act;
					// CTF: 1 2 1:
					// CTF: -1 1 3: RED flag returned after timeout
					if(*rsp(param_CTF, num, col, act) != ':' || col < 1 || col > 2)
						nlog("Error parsing CTF: " << param_CTF);
					else if(!do_read_backlog)
					{
						for(auto plugin: events[KE_CTF])
							plugin->ctf(min, sec, slot(num), col, act);
					}
				}
				else if((*chk_Callvote) == ':')
				{
					if(events[KE_LOG_CALLVOTE].empty())
						continue;

					str type;
					str info;
					// Callvote: 6 map ps37ctf:
					if(*pw(adv(pw(adv(pi(param_Callvote, num), 1), type), 1), info, ':') != ':')
						nlog("Error parsing Callvote: "  << param_Callvote);
					else if(!do_read_backlog)
					{
						for(auto plugin: events[KE_LOG_CALLVOTE])
							plugin->callvote(min, sec, slot(num), type, info);
					}
				}
				else if((*chk_Challenge) == ':')
				{
				}
				else if((*chk_ClientBegin) == ':')
				{
					if(*rsp(param_ClientBegin, num)) // should be zero terminator
						nlog("Error parsing ClientBegin: "  << param_ClientBegin);
					else if(num < 0 || num >= MAX_CLIENTS)
						nlog("ERROR: Client num range: " << num);
					else
					{
						clients[num].live = true;

						if(!do_read_backlog)
							for(auto plugin: events[KE_CLIENT_BEGIN])
								plugin->client_begin(min, sec, slot(num));
					}
				}
				else if((*chk_ClientConnect) == ':')
				{
					if(*rsp(param_ClientConnect, num)) // should be zero terminator
						nlog("Error parsing ClientConnect: " << param_ClientConnect);
					else
					{
						if(num > -1 && num < slot::max)
						{
							clients[num].live = true;

							if(!do_read_backlog)
								for(auto plugin: events[KE_CLIENT_CONNECT])
									plugin->client_connect(min, sec, slot(num));
						}
					}
				}
				else if((*chk_ClientDisconnect) == ':')
				{
					if(*rsp(param_ClientDisconnect, num)) // should be zero terminator
						nlog("Error parsing: "  << cmd);
					else if(num < 0 || num >= MAX_CLIENTS)
						nlog("ERROR: Client num range: " << num);
					else
					{
						clients[num].live = false;

						siz teamBefore = clients[num].team;

						{
							lock_guard lock(mtx);
							clients[num].team = TEAM_U;
						}

						if(!do_read_backlog)
						{
							for(auto plugin: events[KE_CLIENT_DISCONNECT])
								plugin->client_disconnect(min, sec, slot(num));

							if(teamBefore != TEAM_U && !clients[num].bot)
							   for(auto plugin: events[KE_CLIENT_SWITCH_TEAM])
									plugin->client_switch_team(min, sec, slot(num), teamBefore, TEAM_U);
						}

					   clients[num].name.clear();
					}
				}
				else if((*chk_ClientConnectInfo) == ':')
				{
					nlog("ClientConnectInfo: " << param_ClientConnectInfo);

					if(events[KE_CLIENT_CONNECT_INFO].empty())
						break;

					str ip;

					// 2 5E68E970866FC20242482AA396BBD43E 81.101.111.32
					if(*rsp(param_ClientConnectInfo, num, guid, ip)) // null termintor
						nlog("Error parsing ClientConnectInfo: "  << param_ClientConnectInfo);
					else if(!clients[num].live)
					{
						// ignore this event until it occurs at a reliable place
						if(mod_katina == "0.1.1")
							nlog("ERROR: This event should NEVER occur in 0.1.1");
						if(mod_katina >= "0.1.2")
							nlog("ERROR: This event should NEVER occur after 0.1.2");
					}
					else if(num < 0 || num >= MAX_CLIENTS)
						nlog("ERROR: Client num range: " << num);
					else
					{
						GUID guid(guid);
						if(!guid)
							nlog("ERROR: bad guid: " << guid);
						else
						{
							if(clients[num].guid != guid)
								nlog("ERROR: mismatched guids: " << clients[num].guid << " vs " << guid);

							if(!do_read_backlog)
								for(auto plugin: events[KE_CLIENT_CONNECT_INFO])
									plugin->client_connect_info(min, sec, slot(num), guid, ip);
						}
					}
				}
				else if((*chk_ClientUserinfoChanged) == ':')
				{
					// 0 n\Neko\t\2\model\neko\hmodel\neko\c1\2\c2\2\hc\70\w\0\l\0\skill\ 2.00\tt\1\tl\0\id\

					siz team;

					if(*pz(adv(pw(adv(pt(pi(param_ClientUserinfoChanged, num), '\\'), 1), name, '\\'), 3), team) != '\\')
						nlog("Error parsing ClientUserinfoChanged: " << param_ClientUserinfoChanged);
					else if(trim_copy(name).empty())
						nlog("ERROR: name empty: " << param_ClientUserinfoChanged);
					else if(num < 0 || num >= MAX_CLIENTS)
						nlog("ERROR: Client num range: " << num);
					else
					{
//						bug("=================================================");
//						bug("== ClientUserinfoChanged                       ==");
//						bug("==---------------------------------------------==");
//						bug_var(get_line_number());
//						bug_var(num);
//						bug_var(name);
//						bug_var(team);

						rad pos = fnd(param_ClientUserinfoChanged, "\\id\\");
//						bug_var(pos);

						if(!pos)
						{
							log("WARN: possible client_userinfo_bug: " << cmd);
							client_userinfo_bug.set(min, sec, line_data);
						}
						else
						{
							str id;
							if(*pw(pos + 4, id))
							{
								nlog("ERROR: parsing guid" << param_ClientUserinfoChanged);
								break;
							}
//							bug_var(id);
							GUID guid(id.size() == 32 ? GUID(id.substr(24)) : GUID(slot(num)));

//							bug_var(guid);
							if(!guid)
							{
								nlog("ERROR: bad guid: " << param_ClientUserinfoChanged);
								break;
							}

							siz hc = 100;
							if(!(pos = fnd(param_ClientUserinfoChanged, "\\hc\\")))
								nlog("WARN: no handicap info found: " << param_ClientUserinfoChanged);
							else
							{
								if(*(pos = pz(pos + 4, hc)) != '\\')
									nlog("ERROR: Parsing handicap: " << param_ClientUserinfoChanged);
							}

//							bug_var(hc);

							siz teamBefore;

							{
								lock_guard lock(mtx);
								teamBefore = clients[num].team;

//								auto f = clients.find(slot(num));
//								if(f != clients.end())
//								{
//									players.erase(f->second);
//									teams.erase(f->second);
//								}

								clients[num].guid = guid;
								clients[num].team = team;
								clients[num].name = name;
//								bug("XX ADDING PLAYER: " << name);
//								players[guid] = name;
//								teams[guid] = team; // 1 = red, 2 = blue, 3 = spec
							}

							// Sanity check
							siz count = 0;
							for(const auto& c: clients)
								if(c.name == name)
									++count;

							if(count > 1)
								nlog("WARN: adding duplicate name to players: [" << guid << "] " << name);

							if(!do_read_backlog)
							{
								for(auto plugin: events[KE_CLIENT_USERINFO_CHANGED])
									plugin->client_userinfo_changed(min, sec, slot(num), team, guid, name, hc);

								if(team != teamBefore && !clients[num].bot)
									for(auto plugin: events[KE_CLIENT_SWITCH_TEAM])
										plugin->client_switch_team(min, sec, slot(num), teamBefore, team);
							}
						}
					}
				}
				break;

			case 'S':
				// Possible events sorted by frequency
				// most frequent first
				// Speed:
				// SpeedFlag:
				// ShutdownGame:

				if((*chk_Speed) == ':' || (*chk_SpeedFlag) == ':')
				{
					if(events[KE_SPEED].empty())
						continue;

					rad params = (((*chk_Speed) == ':') ? chk_Speed : chk_SpeedFlag) + 2;

					siz dist, time;
					// 0:34 SpeedFlag: 8 1735 4 : Client 8 ran 1735u in 4s while holding the flag.
					// mod_katina <= 0.2 has a bug in Speed: & SpeedFlag: (space beore ':')
					// if(*(iss = rsp(iss, num, dist, time)) != ':') <= so this won't work before mod_katina 0.2.1
					if(*ps(rsp(params, num, dist, time)) != ':')
						nlog("Error parsing " << cmd);
					else if(!do_read_backlog)
					{
						for(auto plugin: events[KE_SPEED])
							plugin->speed(min, sec, slot(num), dist, time, ((*chk_SpeedFlag) == ':'));
					}
				}
				else if((*chk_ShutdownGame) == ':')
				{
					if(!do_read_backlog)
						for(auto plugin: events[KE_SHUTDOWN_GAME])
							plugin->shutdown_game(min, sec);
				}
			break;

			case 'W':
//			{
//				on_scoper on;
//				bug("== W");
//				bug_var(cmd);
				// Possible events sorted by frequency
				// most frequent first
				// WeaponUsage:
				// Warmup:

				if((*chk_Warmup) == ':')
				{
					if(!do_read_backlog)
						for(auto plugin: events[KE_WARMUP])
							plugin->warmup(min, sec);
				}
				else if((*chk_WeaponUsage) == ':')
				{
					// Weapon Usage Update
					// WeaponUsage: <client#> <weapon#> <#shotsFired>
					siz weap, shots;

					if(*rsp(param_WeaponUsage, num, weap, shots))
						nlog("Error parsing WeaponUsage: " << param_WeaponUsage);
					else if(!do_read_backlog)
					{
						for(auto plugin: events[KE_WEAPON_USAGE])
							plugin->weapon_usage(min, sec, slot(num), weap, shots);
					}
				}
//			}
			break;

			case 'M':
				// Possible events sorted by frequency
				// most frequent first
				// MODDamage:

				// MOD (Means of Death = Damage Type) Damage Update
				// MODDamage: <client#> <mod#> <#hits> <damageDone> <#hitsRecv> <damageRecv> <weightedHits>
				siz mod, hits, dmg, hitsRecv, dmgRecv;
				float weightedHits;
				// MODDamage: 1 10 1 560 0 0 1.000000

				if(*rsp(param_MODDamage, num, mod, hits, dmg, hitsRecv, dmgRecv, weightedHits))
					nlog("Error parsing MODDamage: " << param_MODDamage);
				else if(!do_read_backlog)
				{
					for(auto plugin: events[KE_MOD_DAMAGE])
						plugin->mod_damage(min, sec, slot(num), mod, hits, dmg, hitsRecv, dmgRecv, weightedHits);
				}
			break;

			case 'c':
				// Possible events sorted by frequency
				// most frequent first
				// chat:
				// Error parsing client: INFO^1: ^5Use ^3?help ^5to get help on the new features
				//   4:08 chat: ^3INFO^1: ^5If you ^1love ^5a map, say ^3!love map ^5to keep it in the rotation
				if((*chk_chat) == ':')
				{
					if(!do_read_backlog)
						for(auto plugin: events[KE_CHAT])
							plugin->chat(min, sec, param_chat);
				}
				else if((*chk_client) == ':')
				{
					if(*rsp(param_client, num) != ':')
					{
						client = slot::bad;
						nlog("Error parsing client: " << param_client);
					}
				}
			break;

			case 's':
				// Possible events sorted by frequency
				// most frequent first
				// say:
				// score:
				// sayteam:

				if((*chk_say) == ':')
				{
					rad param_text = 0;
//					on_scoper on;
					// say: ^1B^7lood^1y ^0[^1B^7o^1y^0]: :-)
					if(mod_katina_flags & KATINA_SAY & KATINA_MACHINE_ONLY)
					{
						// say: 5: text
						if(*(iss = rsp(param_say, num)) != ':')
							nlog("ERROR: parsing: " << cmd);
						else
						{
							param_text = iss + 2;
						}
					}
					// if KATINA_SAY or if we failed to parse KATINA_MACHINE_ONLY
					else if((mod_katina_flags & KATINA_SAY) || !param_text)
					{
						// say: 5 8 Gurtrude: text
						siz length;

						if((iss = rsp(param_say, num, length)) == param_say)
							nlog("ERROR: parsing: " << cmd);
						else if((*(iss = iss + 1 + length)) != ':')
							nlog("ERROR: parsing: " << cmd);
						else
						{
							param_text = iss + 2;
						}
					}
					else
					{
						if((num = siz(extract_num(param_say))) == siz(slot::bad))
						{
							// FIXME:
//							2015-08-23 22:27:51: ERROR: Unable to locate slot when parsing say: 2 13 UnnamedPlayer: :o {218} [Katina.cpp] (2165)
//							2015-08-23 22:27:51: ERROR: Client num range: -1 {218} [Katina.cpp] (2173)

							nlog("ERROR: Unable to locate slot when parsing say: " << param_say);
						}
						else
						{
							param_text = param_say + clients[num].name.size() + 2;
						}
					}

					if(num < 0 || num >= MAX_CLIENTS)
						nlog("ERROR: Client num range: " << num);
					else
					{
						if(param_text)
						{
							str text = param_text;

							if(!text.find("!katina"))
								builtin_command(slot(num), text);
							else if(!do_read_backlog)
							{
								for(auto plugin: events[KE_SAY])
									plugin->say(min, sec, slot(num), text);
							}
						}
					}
				}
				else if((*chk_score) == ':')
				{
					if(events[KE_SCORE_EXIT].empty())
						continue;

					int score = 0;
					siz ping = 0;
					str name;
					// score: 78  ping: 68  client: 3 ^4ebtwajumac ^3[UA]^7
					if(*p0(psi(pt(pt(pi(param_score, score), ':') + 1, ':') + 1, num) + 1, name))
						nlog("Error parsing SCORE_EXIT:" << param_score);
					else if(!do_read_backlog)
					{
						for(auto plugin: events[KE_SCORE_EXIT])
							plugin->score_exit(min, sec, score, ping, slot(num), name);
					}
				}
				else if((*chk_sayteam) == ':')
				{
					// TODO: implement this
//					if(events[KE_SAYTEAM].empty())
//						continue;
//
//					GUID guid = (client == slot::bad) ? extract_name(param_sayteam) : getClientGuid(client);
//					client = slot::bad;
//
//					if(guid == null_guid)
//						nlog("ERROR: Unable to locate GUID when parsing sayteam: " << line_data);
//					else
//					{
//						const str& name = getPlayerName(guid);
//
//						if(name.empty())
//							nlog("ERROR: Unable to locate name when parsing say: " << param_sayteam);
//						else if(!do_read_back)
//						{
//							str text(param_sayteam + name.size() + 2);
//
//							for(auto plugin: events[KE_SAYTEAM])
//								plugin->say(min, sec, guid, text);
//						}
//					}
				}
			break;

			case 'P':
				// Possible events sorted by frequency
				// most frequent first
				// PlayerScore:
				// Push:
				// Playerstore:
				// PlayerStats:

				if((*chk_Push) == ':')
				{
					if(events[KE_PUSH].empty())
						continue;

					if(*rsp(param_Push, num1, num2) != ':')
						nlog("Error parsing Push:" << param_Push);
					else if(!do_read_backlog)
					{
						for(auto plugin: events[KE_PUSH])
							plugin->push(min, sec, slot(num1), slot(num2));
					}
				}
				else if((*chk_PlayerStats) == ':' && (*(chk_PlayerStats - 1)) == 's')
				{
					//on_scoper on;
					// Player Stats Update
					// PlayerStats: <client#>
					// 				<fragsFace> <fragsBack> <fraggedInFace> <fraggedInBack>
					// 				<spawnKillsDone> <spanwKillsRecv>
					// 				<pushesDone> <pushesRecv>
					// 				<healthPickedUp> <armorPickedUp>
					//				<holyShitFrags> <holyShitFragged>
					siz fragsFace, fragsBack, fraggedFace, fraggedBack, spawnKills, spawnKillsRecv;
					siz pushes, pushesRecv, health, armor, holyShitFrags, holyShitFragged;
					bug_var(param_PlayerStats);
					// TODO: consider separating (health/armour & holyshitfrags/fragged)
					// PlayerStats: 7 0 0 0 0 0 0 1 2 0 0 0 0
					if(*rsp(param_PlayerStats, num,
						fragsFace, fragsBack, fraggedFace, fraggedBack,
						spawnKills, spawnKillsRecv, pushes, pushesRecv,
						health, armor, holyShitFrags, holyShitFragged))
						nlog("Error parsing PlayerStats: " << param_PlayerStats);
					else if(!do_read_backlog)
					{
						for(auto plugin: events[KE_PLAYER_STATS])
						{
							plugin->player_stats(min, sec, slot(num),
								fragsFace, fragsBack, fraggedFace, fraggedBack,
								spawnKills, spawnKillsRecv, pushes, pushesRecv,
								health, armor, holyShitFrags, holyShitFragged);
						}
					}
				}
			break;

			case 'I':
				// Possible events sorted by frequency
				// most frequent first
				// Item:
				// Info:
				// InitGame:

				if((*chk_Item) == ':')
				{
				}
				if((*chk_InitGame) == ':')
				{
					{
						lock_guard lock(mtx);
						svars.clear();
//						teams.clear();
//						clients.clear();
						for(auto&& c: clients)
							c.clear();
						players.clear();

						static str key, val;
						iss = param_InitGame + 1; // skip initial '\\'
						while(*(iss = adv(pw(adv(pw(iss, key, '\\'), 1), val, '\\'), 1)))
							svars[key] = val;
						svars[key] = val;

						mapname = svars["mapname"];
						mod_katina = svars["mod_katina"];
						timestamp = svars["g_timestamp"];

						bug_var(mapname);
						bug_var(mod_katina);
						bug_var(timestamp);
					}

					if(rerun && !have("runtime"))
					{
						siz Y, M, D, h, m, s;
						// g_timestamp 2013-05-24 09:34:32
						if(timestamp.size() != sizeof("0000-00-00 00:00:00") - 1)
							nlog("ERROR: empty g_timestamp: " << param_InitGame);
						if(*pz(pz(pz(pz(pz(pz(timestamp.c_str(), Y) + 1, M) + 1, D) + 1, h) + 1, m) + 1, s))
							nlog("ERROR: parsing g_timestamp: " << timestamp);
						else
						{
							tm t;
							std::time_t _t = std::time(0);
							t = *gmtime(&_t);
							t.tm_year = Y - 1900;
							t.tm_mon = M - 1;
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

					if(!do_read_backlog)
						for(auto plugin: events[KE_INIT_GAME])
							plugin->init_game(min, sec, svars);
				}
			break;

			case 'r':
				// Possible events sorted by frequency
				// most frequent first
				// red:

				if(events[KE_CTF_EXIT].empty())
					continue;

				// red:8  blue:5
				if(*pz(pt(pz(param_red, r), ':') + 1, b)) // EOS
					nlog("Error parsing CTF_EXIT:" << param_red);
				else if(!do_read_backlog)
				{
					for(auto plugin: events[KE_CTF_EXIT])
						plugin->ctf_exit(min, sec, r, b);
				}
			break;

			case 'E':
				// Possible events sorted by frequency
				// most frequent first
				// Exit:

				if(!do_read_backlog)
					for(auto plugin: events[KE_EXIT])
						plugin->exit(min, sec);
			break;

			default:
				// this is way too promiscuous, replace with user-defined events?
//				for(auto plugin: events[KE_UNKNOWN])
//					plugin->unknown(min, sec, cmd, params);
			break;
			}

			// clear some threads
			sys_time_point end = sys_clk::now() + std::chrono::microseconds(1);
			std::future_status fs;
			for(future_lst_iter i = futures.begin(); i != futures.end();)
			{
				if((fs = i->wait_until(end)) == std::future_status::timeout)
					break;

				if(fs == std::future_status::ready)
				{
					if(i->valid())
						i->get();
					i = futures.erase(i);
					continue;
				}
				++i;
			}
		}

		// only process first logfile when
		// running live
		if(!rerun)
			break;
	}

	if(!futures.empty())
		log("Waiting on threads to terminate...");

	for(future_lst_vt& fut: futures)
		if(fut.valid())
			fut.get();

	log("TERMINATE: all good");

	return true;
}

} // katina
