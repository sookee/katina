/*
 * File:   Katina.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 1, 2013, 6:23 PM
 */

#include <dlfcn.h>
#include <cassert>

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

/* void* cvarpoll(void* vp)
{
	Katina& katina = *reinterpret_cast<Katina*>(vp);
	cvarevt_lst& cvarevts = katina.cvarevts;
	cvarevt_lst_iter cvar = cvarevts.begin();
	
	while(!katina.done)
	{
		thread_sleep_millis(3000);

		str value = cvar->value;
		if(!katina.rconset(cvar->name, cvar->value))
			katina.rconset(cvar->name, cvar->value); // one retry

		if(value != cvar->value) // changed
			cvar->plugin->cvar_event(cvar->name, cvar->value);
		
		pthread_mutex_lock(&katina.cvarevts_mtx);
		if(++cvar == cvarevts.end())
			cvar = cvarevts.begin();
		pthread_mutex_unlock(&katina.cvarevts_mtx);
	}
	pthread_exit(0);
}
 */
void* cvarpoll(void* vp)
{
	Katina& katina = *reinterpret_cast<Katina*>(vp);
	cvar_map_map& cvars = katina.cvars;
	
	cvar_map_iter mi;
	cvar_map_map_iter mmi = cvars.end();
	
	while(!katina.done)
	{
		thread_sleep_millis(3000);

		lock_guard lock(katina.cvarevts_mtx);
		
		if(mmi == cvars.end())
		{
			mmi = cvars.begin();
			if(mmi != cvars.end())
				mi = mmi->second.begin();
		}
		
		if(mmi == cvars.end())
			continue;
		
		if(mi == mmi->second.end())
		{
			++mmi;
			continue;
		}
		
		str value;
		str old_value;
		
		mi->second->get(old_value);
		
		if(!katina.rconset(mi->first, value))
			katina.rconset(mi->first, value); // one retry

		if(value != old_value) // changed
			mi->second->set(value);
		
		++mi;
	}
	pthread_exit(0);
}

Katina::Katina()
: done(false)
, active(true) // TODO: make this false
{
	pthread_mutex_init(&cvarevts_mtx, 0);
	pthread_create(&cvarevts_thread, 0, &cvarpoll, (void*) this);
}

Katina::~Katina()
{
	done = true;
	pthread_join(cvarevts_thread, 0);
}
/*
void Katina::add_var_event(KatinaPlugin* plugin, const str& name, const str& value)
{
	cvarevt e;
	e.name = name;
	e.value = value;
	e.plugin = plugin;
	
	pthread_mutex_lock(&cvarevts_mtx);
	if(std::find(cvarevts.begin(), cvarevts.end(), e) == cvarevts.end())
		cvarevts.push_back(e);
	pthread_mutex_unlock(&cvarevts_mtx);
}
*/
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

	str sval;

	if(response.find("unknown command:"))
	{
		str skip;
		siss iss(response);
		if(!std::getline(std::getline(iss, skip, ':').ignore(), sval, '^'))
		{
			log("ERROR: parsing rconset response: " << response);
			return false;
		}
	}

	val = sval;
	return true;
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

bool Katina::chat_to(siz num, const str& text)
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

bool Katina::start(const str& dir)
{
	config_dir = expand_env(dir);
	
	log("Setting config dir: " << dir);

	std::ifstream ifs((config_dir + "/katina.conf").c_str());

	// working variables
	char c;
	siz min, sec;
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

	// load pki

	str keypair_file = get("pki.keypair", "keypair.pki");
	str pkey_file = get("pki.pkey", "pkey.pki");

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
		if(pki.get_public_key_as_text(sexp))
		{
			std::ofstream ofs((config_dir + "/" + pkey_file).c_str());
			ofs << sexp;
		}
	}
	
	// initialize rcon
	
	server.config(get("rcon.host"), get<siz>("rcon.port"), get("rcon.pass"));
		
//	if(!server.command("status"))

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

		str params;
		if(!sgl(iss >> min >> c >> sec >> cmd >> std::ws, params))
		{
			log("ERROR: parsing logfile: " << line);
			continue;
		}

		iss.str(params);
		
		lock_guard lock(cvarevts_mtx);
		
		if(cmd == "Exit:")
		{
			for(plugin_vec_iter i = events[EXIT].begin()
				; i != events[EXIT].end(); ++i)
				(*i)->exit(min, sec);
		}
		else if(cmd == "ShutdownGame:")
		{
			for(plugin_vec_iter i = events[SHUTDOWN_GAME].begin()
				; i != events[SHUTDOWN_GAME].end(); ++i)
				(*i)->shutdown_game(min, sec);
		}
		else if(cmd == "Warmup:")
		{
			for(plugin_vec_iter i = events[WARMUP].begin()
				; i != events[WARMUP].end(); ++i)
				(*i)->warmup(min, sec);
		}
		else if(cmd == "ClientUserinfoChanged:")
		{
			siz num, team;
			if(!(sgl(sgl(sgl(iss >> num, skip, '\\'), name, '\\'), skip, '\\') >> team))
				std::cout << "Error parsing ClientUserinfoChanged: "  << line << '\n';
			else
			{
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
	
					for(plugin_vec_iter i = events[CLIENT_USERINFO_CHANGED].begin()
						; i != events[CLIENT_USERINFO_CHANGED].end(); ++i)
						(*i)->client_userinfo_changed(min, sec, num, team, guid, name);
				}
			}
		}
		else if(cmd == "ClientConnect:")
		{
			bug(cmd);
			siz num;
			if(!(iss >> num))
				std::cout << "Error parsing ClientConnect: "  << line << '\n';
			else
			{
				for(plugin_vec_iter i = events[CLIENT_CONNECT].begin()
					; i != events[CLIENT_CONNECT].end(); ++i)
					(*i)->client_connect(min, sec, num);
			}
		}
		else if(cmd == "ClientDisconnect:")
		{
			bug(cmd);
			siz num;
			if(!(iss >> num))
				std::cout << "Error parsing ClientConnect: "  << line << '\n';
			else
			{
				for(plugin_vec_iter i = events[CLIENT_DISCONNECT].begin()
					; i != events[CLIENT_DISCONNECT].end(); ++i)
					(*i)->client_disconnect(min, sec, num);
			}
		}
		else if(cmd == "Kill:")
		{
			bug(cmd);

			siz num1, num2, weap;
			if(!(iss >> num1 >> num2 >> weap))
				log("Error parsing Kill:" << line);
			else
			{
				for(plugin_vec_iter i = events[KILL].begin()
					; i != events[KILL].end(); ++i)
					(*i)->kill(min, sec, num1, num2, weap);
			}
		}
		else if(cmd == "CTF:")
		{
			bug(cmd);

			siz num, col, act;
			if(!(iss >> num >> col >> act) || col < 1 || col > 2)
				std::cout << "Error parsing CTF:" << '\n';
			else
			{
				for(plugin_vec_iter i = events[CTF].begin()
					; i != events[CTF].end(); ++i)
					(*i)->ctf(min, sec, num, col, act);
			}
		}
		else if(cmd == "Award:")
		{
			siz num, awd;
			if(!(iss >> num >> awd))
				std::cout << "Error parsing Award:" << '\n';
			else
			{
				for(plugin_vec_iter i = events[AWARD].begin()
					; i != events[AWARD].end(); ++i)
					(*i)->award(min, sec, num, awd);
			}
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
				mapname = "unknown";
				std::istringstream iss(line.substr(pos + 8));
				if(!std::getline(iss, mapname, '\\'))
					std::cout << "Error parsing mapname\\" << '\n';
				lower(mapname);
			}
			log("MAP NAME: " << mapname);

			for(plugin_vec_iter i = events[INIT_GAME].begin()
				; i != events[INIT_GAME].end(); ++i)
				(*i)->init_game(min, sec);
		}
		else if(cmd == "say:")
		{
			str text;
			GUID guid;

			if(extract_name_from_text(line, guid, text))
				for(plugin_vec_iter i = events[SAY].begin()
					; i != events[SAY].end(); ++i)
					(*i)->say(min, sec, guid, text);
		}
		else
		{
			for(plugin_vec_iter i = events[UNKNOWN].begin()
				; i != events[UNKNOWN].end(); ++i)
				(*i)->unknown(min, sec, cmd, params);
		}
		//pthread_mutex_unlock(&cvarevts_mtx);
	}
	return true;
}

} // oastats