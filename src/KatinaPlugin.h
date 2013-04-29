/*
 * File:   KatinaPlugin.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 5:08 AM
 */

#ifndef KATINAPLUGIN_H
#define	KATINAPLUGIN_H

#include <memory>

#include "types.h"
#include "GUID.h"

namespace oastats {

using namespace oastats::types;

struct GameInfo
{
	str mapname;
	siz_guid_map clients; // slot -> GUID
	guid_str_map players; // GUID -> name
	guid_siz_map teams; // GUID -> 'R' | 'B'
};

class KatinaPlugin
{
private:
	void* dl;

public:
	KatinaPlugin(): dl(0) {}
	virtual ~KatinaPlugin() {}

	/**
	 * This provides an opportunity for a plugin to initialise
	 * itself.
	 *
	 * @return false on failure
	 */
	virtual bool open(str_map& config) = 0;

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

	virtual bool exit(GameInfo& gi) = 0;
	virtual bool shutdown_game(GameInfo& gi) = 0;
	virtual bool warmup(GameInfo& gi) = 0;
	virtual bool client_userinfo_changed(GameInfo& gi, siz num, siz team, const GUID& guid, const str& name) = 0;
	virtual bool client_connect(GameInfo& gi, siz num) = 0;
	virtual bool client_disconnect(GameInfo& gi, siz num) = 0;
	virtual bool kill(GameInfo& gi, siz num1, siz num2, siz weap) = 0;
	virtual bool ctf(GameInfo& gi, siz num, siz team, siz act) = 0;
	virtual bool award(GameInfo& gi) = 0;
	virtual bool init_game(GameInfo& gi) = 0;
	virtual bool say(GameInfo& gi) = 0;
	virtual bool unknown(GameInfo& gi) = 0;

	/**
	 * This provides an opportunity for a plugin to clean
	 * itself up. It is called when the IrcBot is closed down.
	 * This is a good place to clean up any threads, close files etc.
	 */
	virtual void close() = 0;
};

typedef std::auto_ptr<KatinaPlugin> KatinaPluginAPtr;

} // oastats

#endif	/* KATINAPLUGIN_H */

