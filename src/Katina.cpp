/*
 * File:   Katina.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 1, 2013, 6:23 PM
 */

#include <iosfwd>
#include <dlfcn.h>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/rcon.h>

#include <katina/time.h>
#include <katina/types.h>
#include <katina/utils.h>
#include <katina/str.h>
#include <katina/log.h>
#include <katina/GUID.h>

namespace oastats {

using namespace oastats::log;
using namespace oastats::net;
using namespace oastats::time;
using namespace oastats::types;
using namespace oastats::utils;
using namespace oastats::string;

const str version = "1.0";
const str tag = "dev";

Katina::Katina()
: done(false)
, active(true) // TODO: make this false
{
}

GUID Katina::guid_from_name(const str& name)
{
	for(guid_str_iter i = players.begin(); i != players.end(); ++i)
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
	KatinaPlugin* plugin;

	void* dl = 0;
	KatinaPlugin* (*katina_plugin_factory)(Katina&) = 0;

	log("PLUGIN LOAD: " << file);

	if(!(dl = dlopen(file.c_str(), RTLD_LAZY|RTLD_GLOBAL)))
	{
		log("PLUGIN LOAD: " << dlerror());
		return false;
	}

	if(!(*(void**)&katina_plugin_factory = dlsym(dl, "katina_plugin_factory")))
	{
		log("PLUGIN LOAD: " << dlerror());
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

	log("PLUGIN LOAD: OK");

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
		log("PLUGIN RELOAD: plugin not found: " << id);
		return false;
	}

	str_map_iter f = plugin_files.find(id);

	if(f == plugin_files.end())
	{
		log("PLUGIN RELOAD: plugin file not known: " << id);
		return false;
	}

	if(!unload_plugin(id))
		log("PLUGIN RELOAD: plugin '" << id << "' failed to unload: " << f->second);

	if(!load_plugin(f->second))
	{
		log("PLUGIN RELOAD: plugin '" << id << "' failed to reload: " << f->second);
		return false;
	}

	return true;
}

KatinaPlugin* Katina::get_plugin(const str& id, const str& version)
{
	plugin_map_iter i = plugins.find(id);

	if(i == plugins.end())
	{
		log("get_plugin: plugin not found: " << id);
		return 0;
	}
	if(i->second->get_version() < version)
	{
		log("get_plugin: wrong version found: " << i->second->get_version() << " expected " << version);
		return 0;
	}

	return i->second;
}

bool Katina::start(const str& config)
{
	std::ifstream ifs((expand_env(config) + "/katina.conf").c_str());

	// working variables
	char c;
	siz m, s;
	str skip, name, cmd;
	siss iss;
	siz pos;
	str line, key, val;

	// read in config

	while(sgl(ifs, line))
	{
		if((pos = line.find("//")) != str::npos)
			line.erase(pos);

		trim(line);

		if(line.empty() || line[0] == '#')
			continue;

		siss iss(line);
		if(sgl(sgl(iss, key, ':') >> std::ws, val))
			props[key].push_back(expand_env(val));
	}
	ifs.close();

	// load plugins

	str_vec pluginfiles = get_vec("plugin");
	for(siz i = 0; i < pluginfiles.size(); ++i)
	{
		load_plugin(pluginfiles[i]);
	}

	ifs.open(get("logfile").c_str(), std::ios::ate);

	if(!ifs.is_open())
	{
		log("FATAL: Logfile not found: " << get("logfile"));
		return false;
	}

	std::istream& is = ifs;

	std::ios::streampos gpos = is.tellg();

	while(!done)
	{
		if(!std::getline(is, line) || is.eof())
		{
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
		iss >> m >> c >> s >> cmd;

		if(cmd == "Exit:")
		{
			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->exit();
		}
		else if(cmd == "ShutdownGame:")
		{
			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->shutdown_game();
		}
		else if(cmd == "Warmup:")
		{
			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->warmup();
		}
		else if(cmd == "ClientUserinfoChanged:")
		{
			siz num, team;
			if(!(sgl(sgl(sgl(iss >> num, skip, '\\'), name, '\\'), skip, '\\') >> team))
			{
				std::cout << "Error parsing ClientUserinfoChanged: "  << line << '\n';
				continue;
			}

			siz pos = line.find("\\id\\");
			if(pos != str::npos)
			{
				str id = line.substr(pos + 4, 32);
				GUID guid;

				if(id.size() != 32)
					guid = bot_guid(num);//null_guid;
				else
					guid = to<GUID>(id.substr(24));

				clients[num] = guid;
				teams[clients[num]] = team; // 1 = red, 2 = blue, 3 = spec
				players[clients[num]] = name;

				for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
					i->second->client_userinfo_changed(num, team, guid, name);
			}
		}
		else if(cmd == "ClientConnect:")
		{
			bug(cmd);
			siz num;
			if(!(iss >> num))
			{
				std::cout << "Error parsing ClientConnect: "  << line << '\n';
				continue;
			}
			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->client_connect(num);
		}
		else if(cmd == "ClientDisconnect:")
		{
			bug(cmd);
			siz num;
			if(!(iss >> num))
			{
				std::cout << "Error parsing ClientConnect: "  << line << '\n';
				continue;
			}
			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->client_disconnect(num);
		}
		else if(cmd == "Kill:")
		{
			bug(cmd);

			siz num1, num2, weap;
			if(!(iss >> num1 >> num2 >> weap))
			{
				log("Error parsing Kill:" << line);
				continue;
			}

			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->kill(num1, num2, weap);
		}
		else if(cmd == "CTF:")
		{
			bug(cmd);

			siz num, col, act;
			if(!(iss >> num >> col >> act) || col < 1 || col > 2)
			{
				std::cout << "Error parsing CTF:" << '\n';
				continue;
			}

			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->ctf(num, col, act);
		}
		else if(cmd == "Award:")
		{
			siz num, awd;
			if(!(iss >> num >> awd))
			{
				std::cout << "Error parsing Award:" << '\n';
				continue;
			}

			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->award(num, awd);
		}
		else if(cmd == "InitGame:")
		{
			bug(cmd);

			clients.clear();
			players.clear();
			teams.clear();

			str msg = "^1K^7at^3i^7na ^3Stats System v^7" + version + "^3-" + tag + ".";
			server.cp(msg);

			siz pos;
			if((pos = line.find("mapname\\")) != str::npos)
			{
				mapname.clear();
				std::istringstream iss(line.substr(pos + 8));
				if(!std::getline(iss, mapname, '\\'))
				{
					std::cout << "Error parsing mapname\\" << '\n';
					continue;
				}
				lower(mapname);
			}
			log("MAP NAME: " << mapname);

			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->init_game();
		}
		else if(cmd == "say:")
		{
			str text;
			GUID guid;

			if(extract_name_from_text(line, guid, text))
				for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
					i->second->say(guid, text);
		}
		else
		{
			for(plugin_map_iter i = plugins.begin(); i != plugins.end(); ++i)
				i->second->unknown(line);
		}
	}
	return true;
}

} // oastats