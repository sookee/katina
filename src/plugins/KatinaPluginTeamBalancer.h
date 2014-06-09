#pragma once

#include <katina/KatinaPlugin.h>
#include <katina/Katina.h>

#include <katina/GUID.h>
#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>

#include "KatinaPluginStats.h"


namespace katina { namespace plugin {
    
using namespace katina;
using namespace katina::log;
using namespace katina::data;
using namespace katina::types;


typedef std::map<slot, float> slot_float_map;
typedef slot_float_map::iterator siz_float_map_iter;
typedef slot_float_map::const_iterator siz_float_map_citer;
typedef slot_float_map::value_type slot_float_map_pair;

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
    virtual ~PlayerEvaluation() {}
        
    virtual float calcRating(slot client) = 0;
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
    virtual ~TeamBuilder() {}
        
    // Returns Mapping: Client Slot -> Team
    virtual bool buildTeams(slot_float_map playerRatings, slot_siz_map& destTeams, TeamBuilderEvent event=TB_UNKNOWN, void* payload=NULL) = 0;
};


/*********************************************************
 * Payload data structures for the different events
 */
struct TB_JoinData
{
    slot client;
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
    slot_float_map playerRatings;
    
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
    
    typedef std::map<slot, QueuedChange> queued_changes_map;
    queued_changes_map queuedChanges;
    
    
    
    void rateAllPlayers();
    float ratePlayer(slot client);
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

	virtual str get_id() const override;
	virtual str get_name() const override;
	virtual str get_version() const override;
    
	/**
	 * This provides an opportunity for a plugin to initialise
	 * itself.
	 *
	 * @return false on failure
	 */
	virtual bool open() override;
    
	// Game server log events
	virtual bool init_game(siz min, siz sec, const str_map& cvars) override;
	virtual bool client_disconnect(siz min, siz sec, slot client) override;
    virtual bool client_switch_team(siz min, siz sec, slot num, siz teamBefore, siz teamNow) override;
	virtual bool ctf(siz min, siz sec, slot client, siz team, siz act) override;
	
	/**
	 * Final score of complete CTF game
	 */
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text) override;
	virtual bool exit(siz min, siz sec) override;

	/**
	 * This provides an opportunity for a plugin to clean
	 * itself up. It is called before the plugin is removed/reloaded.
	 * This is a good place to clean up any threads, close files etc.
	 */
	virtual void close() override;
    
    virtual void heartbeat(siz min, siz sec) override;
};


} } // Namespace katina::plugin

