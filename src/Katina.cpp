/*
 * File:   Katina.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 1, 2013, 6:23 PM
 */

#undef DEBUG

#include <dlfcn.h>
#include <cassert>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/rcon.h>

#include <katina/time.h>
#include <katina/types.h>
#include <katina/utils.h>
#include <katina/str.h>
#include <katina/GUID.h>

#include <katina/log.h>

namespace oastats {

using namespace oastats::log;
using namespace oastats::net;
using namespace oastats::time;
using namespace oastats::types;
using namespace oastats::utils;
using namespace oastats::string;

const str version = "1.0";
const str tag = "dev";

str Katina::get_version() { return version + "-" + tag; }

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
			mi = mmi->second.begin();
			continue;
		}
		
		//bug("cvar: " << mi->first);

		str old_value;		
		mi->second->get(old_value);		
		//bug("old: " << old_value);
			
		str value;
		if(katina.rconset(katina.prefix + mi->first, value))
			mi->second->set(value);
		else if(katina.rconset(katina.prefix + mi->first, value)) // one retry
			mi->second->set(value);
		
		mi->second->get(value);
		if(value != old_value) // changed
			log("INFO: cvar: " << (katina.prefix + mi->first) << " changing value: " << value);
		
		++mi;
	}
	pthread_exit(0);
}

Katina::Katina()
: done(false)
, active(true) // TODO: make this false
, logmode(LOG_NORMAL) 
, now(std::time(0))
{
	pthread_mutex_init(&cvarevts_mtx, 0);
}

Katina::~Katina()
{
	done = true;
	pthread_join(cvarevts_thread, 0);
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

	str sval;

	trim(response);
	
	if(response.empty())
		return false;
	
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
			props[key].push_back(expand_env(val, WRDE_SHOWERR|WRDE_UNDEF));
		
	}
	ifs.close();
	log("CONFIG LOAD: OK:");
	return true;
}

bool Katina::start(const str& dir)
{
	config_dir = expand_env(dir);
	
	log("Setting config dir: " << dir);

	load_config(config_dir, "katina.conf", props);
	
	// load pki

	log("Reading keypair:");

	str keypair_file = get("pki.keypair", "keypair.pki");
	str pkeys_file = get("pki.pkeys", "pkeys.pki");

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
			std::ofstream ofs((config_dir + "/" + pkeys_file).c_str());
			ofs << sexp;
		}
	}
	
	log("Reading public keys:");
	// TOSO: ad this
	//ifs.open((config_dir + "/" + pkeys_file).c_str());
	//while(pki.read_public_key(, ifs)) {}
	//ifs.close();
	
	// initialize rcon
	
	log("Initializing rcon:");

	server.config(get("rcon.host"), get<siz>("rcon.port"), get("rcon.pass"));
	
	if(get("rcon.active") == "true")
		server.on();

	// load plugins

	log("Loading plugins:");

	str_vec pluginfiles = get_vec("plugin");
	for(siz i = 0; i < pluginfiles.size(); ++i)
		load_plugin(pluginfiles[i]);

	prefix = get("rcon.cvar.prefix");
	if(!prefix.empty())
		prefix += ".";
	pthread_create(&cvarevts_thread, 0, &cvarpoll, (void*) this);
	
	std::ios::openmode mode = std::ios::in|std::ios::ate;

	bool rerun = get("run.mode") == "rerun";
	
	if(rerun)
		mode = std::ios::in;

	log("Opening logfile (" << get("run.mode") << "): " << get("logfile"));

	std::ifstream ifs;
	
	ifs.open(get("logfile").c_str(), mode);

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

		str params;
		if(!sgl(sgl(iss >> min >> c >> sec >> std::ws, cmd, ':') >> std::ws, params))
		{
			if(client_userinfo_bug)
			{
				log("WANING: possible ClientUserinfoChanged bug");
				// \c2\ \hc\100\w\0\l\0\tt\0\tl\0\id\041BD1732752BCC408FAF45616A8F64B
				siz pos;
				if((pos = line.find("\\id\\")) == str::npos)
				{
					log("ERROR: parsing logfile: " << line);
					continue;
				}
				else
				{
					log("ALERT: ClientUserinfoChanged bug detected");
					// 2 n\^1S^2oo^3K^5ee\t\3\mo
					cmd = "ClientUserinfoChanged";
					params = client_userinfo_bug.params + line;
					log("     : cmd   : " << cmd);
					log("     : params: " << params);
				}
			}			
		}
		
		cmd += ":";
		
//		bug_var(cmd);
//		bug_var(params);
		
		client_userinfo_bug.reset();

		iss.clear();
		iss.str(params);
		
		lock_guard lock(cvarevts_mtx);
		
		if(rerun)
			++now;
		else
			now = std::time(0);
		
		if(cmd == "Exit:")
		{
			if(events[EXIT].empty())
				continue;
			
			bug(cmd << "(" << params << ")");

			for(plugin_vec_iter i = events[EXIT].begin()
				; i != events[EXIT].end(); ++i)
				(*i)->exit(min, sec);
		}
		else if(cmd == "ShutdownGame:")
		{
			if(events[SHUTDOWN_GAME].empty())
				continue;
			
			bug(cmd << "(" << params << ")");

			for(plugin_vec_iter i = events[SHUTDOWN_GAME].begin()
				; i != events[SHUTDOWN_GAME].end(); ++i)
				(*i)->shutdown_game(min, sec);
		}
		else if(cmd == "Warmup:")
		{
			if(events[WARMUP].empty())
				continue;
			
			bug(cmd << "(" << params << ")");

			for(plugin_vec_iter i = events[WARMUP].begin()
				; i != events[WARMUP].end(); ++i)
				(*i)->warmup(min, sec);
		}
		else if(cmd == "ClientUserinfoChanged:")
		{
			if(events[CLIENT_USERINFO_CHANGED].empty())
				continue;
			
			bug(cmd << "(" << params << ")");
			
			// 0 n\Merman\t\2\model\merman\hmodel\merman\c1\1\c2\1\hc\70\w\0\l\0\skill\ 2.00\tt\0\tl\0\id\
			// 2 \n\^1S^2oo^3K^5ee\t\3\c2\d\hc\100\w\0\l\0\tt\0\tl\0\id\041BD1732752BCC408FAF45616A8F64B
			siz num, team;
			if(!(sgl(sgl(sgl(iss >> num, skip, '\\'), name, '\\'), skip, '\\') >> team))
				std::cout << "Error parsing ClientUserinfoChanged: "  << params << '\n';
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
			if(events[CLIENT_CONNECT].empty())
				continue;
			
			bug(cmd << "(" << params << ")");

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
		else if(cmd == "ClientDisconnect:")
		{
			if(events[CLIENT_DISCONNECT].empty())
				continue;
			
			bug(cmd << "(" << params << ")");

			siz num;
			if(!(iss >> num))
				std::cout << "Error parsing ClientConnect: "  << params << '\n';
			else
			{
				for(plugin_vec_iter i = events[CLIENT_DISCONNECT].begin()
					; i != events[CLIENT_DISCONNECT].end(); ++i)
					(*i)->client_disconnect(min, sec, num);
					
				teams.erase(clients[num]);
				players.erase(clients[num]);
				clients.erase(num);
			}
		}
		else if(cmd == "Kill:")
		{
			if(events[KILL].empty())
				continue;
			
			bug(cmd << "(" << params << ")");

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
		else if(cmd == "WeaponUsage:")
		{
			bug(cmd << "(" << params << ")");
		
			// Weapon Usage Update
			// WeaponUsage: <client#> <weapon#> <#shotsFired>
			siz num, weap, shots;
			
			if(iss >> num >> weap >> shots)
			{
				for(plugin_vec_iter i = events[WEAPON_USAGE].begin(); i != events[WEAPON_USAGE].end(); ++i)
					(*i)->weapon_usage(min, sec, num, weap, shots);
			}
			else
				std::cout << "Error parsing WeaponUsage" << '\n';
		}
		else if(cmd == "MODDamage:")
		{
			bug(cmd << "(" << params << ")");
		
			// MOD (Means of Death = Damage Type) Damage Update
			// MODDamage: <client#> <mod#> <#hits> <damageDone> <#hitsRecv> <damageRecv>
			siz num, mod, hits, dmg, hitsRecv, dmgRecv;
			if(iss >> num >> mod >> hits >> dmg >> hitsRecv >> dmgRecv)
			{
				for(plugin_vec_iter i = events[MOD_DAMAGE].begin(); i != events[MOD_DAMAGE].end(); ++i)
					(*i)->mod_damage(min, sec, num, mod, hits, dmg, hitsRecv, dmgRecv);
			}
			else
				std::cout << "Error parsing MODDamage" << '\n';
		}
		else if(cmd == "PlayerStats:")
		{
			bug(cmd << "(" << params << ")");
		
			// Player Stats Update
			// PlayerStats: <client#>
			// 			    <fragsFace> <fragsBack> <fraggedInFace> <fraggedInBack>
			// 			    <spawnKillsDone> <spanwKillsRecv>
			// 			    <pushesDone> <pushesRecv>
			// 			    <healthPickedUp> <armorPickedUp>
			siz num, fragsFace, fragsBack, fraggedFace, fraggedBack, spawnKills, spawnKillsRecv, pushes, pushesRecv, health, armor;
			if(iss >> num >> fragsFace >> fragsBack >> fraggedFace >> fraggedBack >> spawnKills >> spawnKillsRecv >> pushes >> pushesRecv >> health >> armor)
			{
				for(plugin_vec_iter i = events[PLAYER_STATS].begin(); i != events[PLAYER_STATS].end(); ++i)
				{
					(*i)->player_stats(min, sec, num,
						fragsFace, fragsBack, fraggedFace, fraggedBack,
						spawnKills, spawnKillsRecv, pushes, pushesRecv,
						health, armor);
				}
			}
			else
				std::cout << "Error parsing PlayerStats" << '\n';
		}
		else if(cmd == "CTF:")
		{
			if(events[CTF].empty())
				continue;
			
			bug(cmd << "(" << params << ")");

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
			if(events[CTF_EXIT].empty())
				continue;
			
			// 20:44 red:4  blue:3
			bug(cmd << "(" << params << ")");
			//bug_var(iss.str());
			siz r = 0;
			siz b = 0;
			str skip;
			if(!(sgl(iss >> r >> std::ws, skip, ':') >> b))
				log("Error parsing CTF_EXIT:" << params);
			else
			{
				bug_var(r);
				bug_var(skip);
				bug_var(b);
				for(plugin_vec_iter i = events[CTF_EXIT].begin()
					; i != events[CTF_EXIT].end(); ++i)
					(*i)->ctf_exit(min, sec, r, b);
			}
		}
		else if(cmd == "score:") // 
		{
			bug(cmd << "(" << params << ")");
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
				bug_var(score);
				bug_var(ping);
				bug_var(num);
				bug_var(name);
				for(plugin_vec_iter i = events[SCORE_EXIT].begin()
					; i != events[SCORE_EXIT].end(); ++i)
					(*i)->score_exit(min, sec, score, ping, num, name);
			}
 		}
		else if(cmd == "Award:")
		{
			if(events[AWARD].empty())
				continue;
			
			bug(cmd << "(" << params << ")");

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
			if(events[INIT_GAME].empty())
				continue;
			
			bug(cmd << "(" << params << ")");

			static str key, val;
			static str_map cvars;
			
			cvars.clear();
			iss.ignore(); // skip initial '\\'
			while(sgl(sgl(iss, key, '\\'), val, '\\'))
				cvars[key] = val;
			
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
				(*i)->init_game(min, sec, cvars);
		}
		else if(cmd == "ClientBegin:")
		{
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
		else if(cmd == "sayteam:")
		{
		}
		else if(cmd == "say:")
		{
			if(events[SAY].empty())
				continue;
			
			bug(cmd << "(" << params << ")");
			
			str text;
			GUID guid;

			if(extract_name_from_text(line, guid, text))
				for(plugin_vec_iter i = events[SAY].begin()
					; i != events[SAY].end(); ++i)
					(*i)->say(min, sec, guid, text);
		}
		else
		{
			if(events[UNKNOWN].empty())
				continue;
			
			bug("UNKNOWN: " << cmd << "(" << params << ")");

			for(plugin_vec_iter i = events[UNKNOWN].begin()
				; i != events[UNKNOWN].end(); ++i)
				(*i)->unknown(min, sec, cmd, params);
		}
		//pthread_mutex_unlock(&cvarevts_mtx);
	}
	return true;
}

} // oastats