#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_H
#define	_OASTATS_KATINA_PLUGIN_H
/*
 * File:   KatinaPlugin.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 5:08 AM
 */

#include <memory>
#include <vector>

//#include <boost/shared_ptr.hpp>

#include "types.h"
#include "GUID.h"

namespace oastats {

using namespace oastats::types;

//struct GameInfo
//{
//	str mapname;
//	siz_guid_map clients; // slot -> GUID
//	guid_str_map players; // GUID -> name
//	guid_siz_map teams; // GUID -> 'R' | 'B'
//};

class KatinaPlugin
{
	friend class Katina;

private:
	void* dl;

protected:
	class Katina& katina;

public:
	KatinaPlugin(Katina& katina): dl(0), katina(katina) {}
	virtual ~KatinaPlugin() {}

	/**
	 * This provides an opportunity for a plugin to initialise
	 * itself.
	 *
	 * @return false on failure
	 */
	virtual bool open() = 0;

	/**
	 * Return the unique id of the plugin.
	 */
	virtual str get_id() const = 0;

	/**
	 * Return the name of the plugin.
	 */
	virtual str get_name() const = 0;

	/**
	 * Return the version of the plugin.
	 */
	virtual str get_version() const = 0;

	// Game server log events
	virtual bool exit() {}
	virtual bool shutdown_game() {}
	virtual bool warmup() {}
	virtual bool client_userinfo_changed(siz num, siz team, const GUID& guid, const str& name) {}
	virtual bool client_connect(siz num) {}
	virtual bool client_disconnect(siz num) {}
	virtual bool kill(siz num1, siz num2, siz weap) {}
	virtual bool ctf(siz num, siz team, siz act) {}
	virtual bool award(siz num, siz awd) {}
	virtual bool init_game() {}
	virtual bool say(const GUID& guid, const str& text) {}
	virtual bool unknown(const str& line) {}

	/**
	 * This provides an opportunity for a plugin to clean
	 * itself up. It is called when the IrcBot is closed down.
	 * This is a good place to clean up any threads, close files etc.
	 */
	virtual void close() = 0;
};

//typedef boost::shared_ptr<KatinaPlugin> KatinaPluginSPtr;
typedef KatinaPlugin* KatinaPluginSPtr;

typedef std::vector<KatinaPluginSPtr> plugin_vec;
typedef plugin_vec::iterator plugin_vec_iter;
typedef plugin_vec::const_iterator plugin_vec_citer;

/**
 * The plugin implementation source should
 * use this macro to make itself loadable as
 * a plugin.
 *
 * The macro variable should be set to the name
 * of the plugin class to instantiate.
 *
 * The plugin class should derive from KatinaPlugin.
 *
 */

#define KATINA_PLUGIN(name) \
extern "C" KatinaPluginSPtr katina_plugin_factory(Katina& katina) \
{ \
	return KatinaPluginSPtr(new name(katina)); \
} extern int _missing_semicolon_()

#define KATINA_PLUGIN_INFO(I, N, V) \
static const char* ID = I; \
static const char* NAME = N; \
static const char* VERSION = V

} // oastats

#endif // _OASTATS_KATINA_PLUGIN_H

