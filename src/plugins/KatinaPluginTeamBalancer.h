#pragma once

#include <katina/KatinaPlugin.h>
#include <katina/Katina.h>

#include <katina/GUID.h>
#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>

#include "KatinaPluginStats.h"


namespace katina { namespace plugin {
    
using namespace oastats;
using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;


typedef std::map<siz, float> siz_float_map;
typedef siz_float_map::iterator siz_float_map_iter;
typedef siz_float_map::const_iterator siz_float_map_citer;
typedef std::pair<const siz, float> siz_float_pair;

typedef std::list<guid_stat_map> stat_list;
typedef stat_list::iterator stat_list_iter;
typedef stat_list::const_iterator stat_list_citer;

class KatinaPluginTeamBalancer;



/*********************************************************
 * Base class for player evaluation algorithms
 * that calculate a rating for a given client slot
 */

class PlayerEvaluation
{
protected:
    Katina& katina;
    KatinaPluginTeamBalancer& balancerPlugin;
    
public:
    PlayerEvaluation(Katina& katina, KatinaPluginTeamBalancer& plugin) :
        katina(katina), balancerPlugin(plugin) {}
        
    virtual float calcRating(siz client) = 0;
};



/*********************************************************
 * Events that call the TeamBuilder methods.
 */
enum TeamBuilderEvent
{
    TB_INIT,
    TB_JOIN,
    TB_DISCONNECT,
    TB_CAPTURE,
    TB_COMMAND,
    TB_UNKNOWN
};



/*********************************************************
 * Base class for team building algorithms
 * that decide how the teams should look.
 */

class TeamBuilder
{
protected:
    Katina& katina;
    KatinaPluginTeamBalancer& balancerPlugin;
    
    bool is1vs1() const;
    
    
public:
    TeamBuilder(Katina& katina, KatinaPluginTeamBalancer& plugin) :
        katina(katina), balancerPlugin(plugin) {}
        
    // Returns Mapping: Client Slot -> Team
    virtual bool buildTeams(siz_float_map playerRatings, siz_map& destTeams, TeamBuilderEvent event=TB_UNKNOWN, void* payload=NULL) = 0;  
};


/*********************************************************
 * Payload data structures for the different events
 */
struct TB_JoinData
{
    siz client;
    siz teamBefore;
    siz teamNow;
};


/*********************************************************
 * Main class of the TeamBalancer Plugin
 * - Receives game events
 * - Triggers recalculation of player ratings
 *   (Via implementations of PlayerEvaluation)
 * - Triggers building of even teams
 *   (Via implementations of TeamBuilder)
 * - Keeps player stats for the last n games
 */

class KatinaPluginTeamBalancer : public KatinaPlugin
{
private:
	bool enabled;
    RCon& rcon;
    KatinaPluginStats* statsPlugin;
    
    PlayerEvaluation* evaluation;
    TeamBuilder* teamBuilderInit;
    TeamBuilder* teamBuilderJoin;
    TeamBuilder* teamBuilderLeave;
    TeamBuilder* teamBuilderCapture;
    
    // Mapping: Client Slot -> Rating
    siz_float_map playerRatings;
    
    siz numLastStats;
    stat_list lastStats;
    
    siz lastBalancing;
    //siz minTimeBetweenRebalancing;
    
    siz scoreRed;
    siz scoreBlue;
    std::list<siz> teamScoreHistory;
    
    bool activeInBotGames;
    
    
    struct QueuedChange
    {
        siz timestamp;
        siz targetTeam;
    };
    
    typedef std::map<siz, QueuedChange> queued_changes_map;
    queued_changes_map queuedChanges;
    
    
    
    void rateAllPlayers();
    float ratePlayer(siz client);
    void buildTeams(TeamBuilder* teamBuilder, TeamBuilderEvent event=TB_UNKNOWN, void* payload=NULL, siz waitTime=15, bool force=false);
    
    void printTeams(bool printBug=false, bool printChat=true);
    
    
public:
    KatinaPluginTeamBalancer(Katina& katina);
    virtual ~KatinaPluginTeamBalancer();
    
    KatinaPluginStats* getStatsPlugin() { return statsPlugin; }
    stat_list& getLastStats() { return lastStats; }
    
    siz getScoreRed() const { return scoreRed; }
    siz getScoreBlue() const { return scoreBlue; }
    const std::list<siz>& getTeamScoreHistory() const { return teamScoreHistory; }

    //
	// INTERFACE: KatinaPlugin
    //

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;
    
	/**
	 * This provides an opportunity for a plugin to initialise
	 * itself.
	 *
	 * @return false on failure
	 */
	virtual bool open();
    
	// Game server log events
	virtual bool init_game(siz min, siz sec, const str_map& cvars);
	//virtual bool warmup(siz min, siz sec) {}
	//virtual bool client_connect(siz min, siz sec, siz client) {}
	//virtual bool client_begin(siz min, siz sec, siz client) {}
	virtual bool client_disconnect(siz min, siz sec, siz client);
	//virtual bool client_userinfo_changed(siz min, siz sec, siz client, siz team, const GUID& guid, const str& name) {}
    virtual bool client_switch_team(siz min, siz sec, siz num, siz teamBefore, siz teamNow);
	//virtual bool kill(siz min, siz sec, siz client1, siz client2, siz weap) {}
	virtual bool ctf(siz min, siz sec, siz client, siz team, siz act);
	
	/**
	 * Final score of complete CTF game
	 */
	//virtual bool ctf_exit(siz min, siz sec, siz r, siz b) {}
	//virtual bool score_exit(siz min, siz sec, int score, siz ping, siz client, const str& name) {}
	//virtual bool award(siz min, siz sec, siz client, siz awd) {}
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
	//virtual bool sayteam(siz min, siz sec, const GUID& guid, const str& text);
	//virtual bool shutdown_game(siz min, siz sec) {}
	virtual bool exit(siz min, siz sec);
	//virtual bool unknown(siz min, siz sec, const str& cmd, const str& params) {}
	
	/**
	 * Summarizing events for more detailed statistics (they only work with the katina game mod)
	 */
	/*virtual bool weapon_usage(siz min, siz sec, siz client, siz weapon, siz shots) {}
	virtual bool mod_damage(siz min, siz sec, siz client, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits) {}
	virtual bool player_stats(siz min, siz sec, siz client,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged) {}
     */
	 
	 
	/**
	 * This provides an opportunity for a plugin to clean
	 * itself up. It is called before the plugin is removed/reloaded.
	 * This is a good place to clean up any threads, close files etc.
	 */
	virtual void close();
    
    virtual void heartbeat(siz min, siz sec);
};


} } // Namespace katina::plugin

