/*
 * File:   Katina.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 1, 2013, 6:23 PM
 */

#undef DEBUG

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

namespace oastats {

using namespace oastats::log;
using namespace oastats::net;
using namespace oastats::time;
using namespace oastats::types;
using namespace oastats::utils;
using namespace oastats::string;

const str version = "0.1";
const str tag = "dev";


siz Katina::getTeam(siz client)
{
    siz_guid_map_citer clientsIt = clients.find(client);
    if(clientsIt == clients.end())
        return TEAM_U;
    
    guid_siz_map_citer teamsIt = teams.find(clientsIt->second);
    if(teamsIt == teams.end())
        return TEAM_U;
    
    return teamsIt->second;
}


str Katina::getPlayerName(siz client)
{
	return players[clients[client]];
//    siz_guid_map_citer clientsIt = clients.find(client);
//    if(clientsIt == clients.end())
//        return "";
//
//    guid_str_map_citer playersIt = players.find(clientsIt->second);
//    if(playersIt == players.end())
//        return "";
//
//    return playersIt->second;
}


siz Katina::getClientNr(GUID guid)
{
    for(siz_guid_map_citer it = clients.begin(); it != clients.end(); ++it)
    {
        if(it->second == guid)
            return it->first;
    }
    
    return siz(-1);
}


str Katina::get_version() { return version + "-" + tag; }



void* cvarpoll(void* vp)
{
	Katina& katina = *reinterpret_cast<Katina*>(vp);
	cvar_map_map& cvars = katina.vars;

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

bool Katina::is_admin(const GUID& guid)
{
	str_vec admins = get_vec("admin.guid");
	for(str_vec_iter i = admins.begin(); i != admins.end(); ++i)
		if(guid == *i)
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

	str cmd;
	siss iss(text);

	static str prefix = "";

	if(!(iss >> cmd))
		log("Error: parsing builtin command: " << text);
	else
	{
		bug_var(cmd);
		if(cmd == "prefix")
		{
			if(iss >> prefix)
				server.s_chat("prefix is now: " + prefix);
		}
		else if(cmd == "set")
		{
			// !katina set plugin varname value
			str plugin, var, val;
			if(!(iss >> plugin >> var >> val))
				log("Error: parsing builtin command parameters: " << text);
			else
			{
				if(!prefix.empty())
					plugin = prefix + "::" + plugin;
				if(!plugins[plugin])
					server.s_chat("Plugin " + plugin + " is not loaded");
				else
				{
					if(!vars[plugins[prefix]][var])
						server.s_chat("Plugin " + plugin + " does not recognise " + var);
					else
					{
						vars[plugins[prefix]][var]->set(val);
						if(vars[plugins[prefix]][var]->get(val))
							server.s_chat("Variable " + var + " set to: " + val);
					}
				}
			}
		}
	}
}

bool Katina::start(const str& dir)
{
	config_dir = expand_env(dir);
	log("Setting config dir: " << dir);

	load_config(config_dir, "katina.conf", props);
    if(!init_pki()) return false;
    init_rcon();
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

	// working variables
	char c;
	siz min, sec;
	str line, skip, name, cmd;
	siss iss;

	now = get("run.time", std::time_t(-1));
	std::time_t base_now = now; // rerun base time

	while(!done)
	{
		//bug("loop:");
		if(!std::getline(is, line) || is.eof())
		{
			if(rerun)
				done = true;
			thread_sleep_millis(50);
			is.clear();
			is.seekg(gpos);
			continue;
		}

		bug_var(line);

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
			continue;

		cmd += ":";

		str params;

		sgl(iss, params); // not all commands have params

		iss.clear();
		iss.str(params);

		lock_guard lock(cvarevts_mtx);

		if(rerun)
			now = base_now + (min * 60) + sec;
		else
			now = std::time(0);
        
        // Send HEARTBEAT event to plugins
        for(plugin_vec_iter i = events[HEARTBEAT].begin(); i != events[HEARTBEAT].end(); ++i)
            (*i)->heartbeat(min, sec);

        bool flagspeed = false; // speed carrying a flag

		if(cmd == "Exit:")
		{
			bug(cmd << "(" << params << ")");
			if(events[EXIT].empty())
				continue;

			for(plugin_vec_iter i = events[EXIT].begin()
				; i != events[EXIT].end(); ++i)
				(*i)->exit(min, sec);
		}
		else if(cmd == "ShutdownGame:")
		{
			bug(cmd << "(" << params << ")");
			if(events[SHUTDOWN_GAME].empty())
				continue;

			for(plugin_vec_iter i = events[SHUTDOWN_GAME].begin()
				; i != events[SHUTDOWN_GAME].end(); ++i)
				(*i)->shutdown_game(min, sec);
		}
		else if(cmd == "Warmup:")
		{
			bug(cmd << "(" << params << ")");
			if(events[WARMUP].empty())
				continue;

			for(plugin_vec_iter i = events[WARMUP].begin()
				; i != events[WARMUP].end(); ++i)
				(*i)->warmup(min, sec);
		}
		else if(cmd == "ClientUserinfoChanged:")
		{
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

					clients[num] = guid;
					players[guid] = name;
                    
                    siz teamBefore = teams[guid];
                    teams[guid] = team; // 1 = red, 2 = blue, 3 = spec

					for(plugin_vec_iter i = events[CLIENT_USERINFO_CHANGED].begin(); i != events[CLIENT_USERINFO_CHANGED].end(); ++i)
						(*i)->client_userinfo_changed(min, sec, num, team, guid, name, hc);
                    
                    if(team != teamBefore && !guid.is_bot())
                    {
                        for(plugin_vec_iter i = events[CLIENT_SWITCH_TEAM].begin(); i != events[CLIENT_SWITCH_TEAM].end(); ++i)
                            (*i)->client_switch_team(min, sec, num, teamBefore, team);
                    }
				}
			}
		}
		else if(cmd == "ClientConnect:")
		{
			bug(cmd << "(" << params << ")");
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
			bug(cmd << "(" << params << ")");
			if(events[CLIENT_CONNECT_INFO].empty())
				continue;

			siz num;
			str ip;
			GUID guid;
			if(!(iss >> num >> guid >> ip))
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
			bug(cmd << "(" << params << ")");
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
			bug(cmd << "(" << params << ")");

			siz num;
			if(!(iss >> num))
				std::cout << "Error parsing ClientConnect: "  << params << '\n';
			else
			{
                // Remove the data first, client is not available in event handlers
                GUID guid = clients[num];
				teams.erase(guid);
				//players.erase(guid); // TODO: WHY IS THIS COMMENTED OUT?
				clients.erase(num);
                
				for(plugin_vec_iter i = events[CLIENT_DISCONNECT].begin()
					; i != events[CLIENT_DISCONNECT].end(); ++i)
					(*i)->client_disconnect(min, sec, num);
			}
		}
		else if(cmd == "Kill:")
		{
			bug(cmd << "(" << params << ")");
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
				log("Error parsing WeaponUsage: " << params);
		}
		else if(cmd == "MODDamage:")
		{
			bug(cmd << "(" << params << ")");

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
			bug(cmd << "(" << params << ")");

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
			bug(cmd << "(" << params << ")");
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
				bug_var(score);
				bug_var(ping);
				bug_var(num);
				bug_var(name);
				for(plugin_vec_iter i = events[SCORE_EXIT].begin()
					; i != events[SCORE_EXIT].end(); ++i)
					(*i)->score_exit(min, sec, score, ping, num, name);
			}
 		}
		else if((flagspeed = cmd == "SpeedFlag:") || (cmd == "Speed:")) // zim@openmafia >= 0.1-beta
		{
			// 9:35 Speed: 3 1957 13 : Client 3 ran 1957u in 13s without the flag.
			// 9:35 SpeedFlag: 3 3704 12 : Client 3 ran 3704u in 12s while holding the flag.
			bug(cmd << "(" << params << ")");
			if(events[SPEED].empty())
				continue;

			siz num, dist, time;
			if(!(iss >> num >> dist >> time))
				log("Error parsing Speed:" << params);
			else
			{
				for(plugin_vec_iter i = events[SPEED].begin()
					; i != events[SPEED].end(); ++i)
					(*i)->speed(num, dist, time, flagspeed);
			}
		}
		else if(cmd == "Award:")
		{
			bug(cmd << "(" << params << ")");
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
			bug(cmd << "(" << params << ")");

			static str key, val;

			clients.clear();
			players.clear();
			teams.clear();
			cvars.clear();

			iss.ignore(); // skip initial '\\'
			while(sgl(sgl(iss, key, '\\'), val, '\\'))
				cvars[key] = val;

			mapname = cvars["mapname"];

			if(rerun)
			{
				str skip;
				siz Y, M, D, h, m, s;
				char c;
				siss iss(cvars["g_timestamp"]);
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

			str msg = "^1K^7at^3i^7na ^3Stats System v^7" + version + "^3-" + tag + ".";
			server.cp(msg);

			log("MAP NAME: " << mapname);

			if(events[INIT_GAME].empty())
				continue;

			for(plugin_vec_iter i = events[INIT_GAME].begin()
				; i != events[INIT_GAME].end(); ++i)
				(*i)->init_game(min, sec, cvars);
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
			if(events[SAYTEAM].empty())
				continue;

			bug(cmd << "(" << params << ")");

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

			bug(cmd << "(" << params << ")");

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
