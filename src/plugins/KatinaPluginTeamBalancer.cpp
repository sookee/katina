#include "KatinaPluginTeamBalancer.h"
#include "TB_DefaultEvaluation.h"
#include "TB_DefaultTeamBuilder.h"


namespace katina { namespace plugin {
    
using namespace oastats;
using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

    

KATINA_PLUGIN_TYPE(KatinaPluginTeamBalancer);
KATINA_PLUGIN_INFO("katina::teambalancer", "katina Team Balancer", "0.1-dev");

const str TEAM_CHAR[] = { "s", "r", "b", "s" };


    
KatinaPluginTeamBalancer::KatinaPluginTeamBalancer(Katina& katina) : 
    KatinaPlugin(katina),
    rcon(katina.server),
    statsPlugin(NULL),
    numLastStats(3),
    lastBalancing(0),
    minTimeBetweenRebalancing(20)
{
    evaluation          = new DefaultEvaluation(katina, *this);
    
    teamBuilderInit     = new DefaultTeamBuilder(katina, *this);
    teamBuilderJoin     = new DefaultTeamBuilder(katina, *this);
    teamBuilderLeave    = new DefaultTeamBuilder(katina, *this);
    teamBuilderCapture  = new DefaultTeamBuilder(katina, *this);;
}


KatinaPluginTeamBalancer::~KatinaPluginTeamBalancer()
{
    delete evaluation;
    delete teamBuilderInit;
    delete teamBuilderJoin;
    delete teamBuilderLeave;
    delete teamBuilderCapture;
}


void KatinaPluginTeamBalancer::rateAllPlayers()
{
    playerRatings.clear();
    for(siz_guid_map_citer it = katina.clients.begin(); it != katina.clients.end(); ++it)
        ratePlayer(it->first);
}


float KatinaPluginTeamBalancer::ratePlayer(siz client)
{
    if(statsPlugin)
        statsPlugin->updatePlayerTime(client);
    
    return playerRatings[client] = evaluation->calcRating(client);
}


void KatinaPluginTeamBalancer::buildTeams(TeamBuilder* teamBuilder)
{
    if(lastBalancing > katina.now - minTimeBetweenRebalancing)
        return;

    // TODO:
    str botSkill;
    //rcon.command("g_spskill", botSkill);
    //rcon.chat("BotSkill Reply: " + botSkill);
    botSkill = "2";
    
    // Call team building algorithm
    siz_map teams;
    if(!teamBuilder->buildTeams(playerRatings, teams))
        return;
    
    // Switch all players via rcon
    for(siz_map_citer it = teams.begin(); it != teams.end(); ++it)
    {
        // Skip player if he's already in the right team
        if(it->second == katina.getTeam(it->first))
            continue;
        
        soss oss;
        
        // Don't move bots, but remove and re-add them
        // Moving bots with !putteam is buggy: http://openarena.ws/board/index.php?topic=3578.msg36081#msg36081
        if(katina.clients[it->first].is_bot())
        {
            str botname = katina.getPlayerName(it->first);
            
            // Remove bot
            oss << "kick " << it->first;
            rcon.command(oss.str());
            
            // Check for valid team nr and re-add bot
            if(it->second == 1 || it->second == 2)
                rcon.command("addbot " + botname + " " + botSkill + " " + TEAM_CHAR[it->second]);
        }
        
        // Move player with !putteam command
        else
        {
            oss << "!putteam " << it->first;

            // Validate returned value for team
            if(it->second <= 3)
                oss << " " << TEAM_CHAR[it->second];
            else
                oss << " s";

            rcon.command(oss.str());
        }
    }
    
    lastBalancing = katina.now;
}


str KatinaPluginTeamBalancer::get_id() const        { return ID; }
str KatinaPluginTeamBalancer::get_name() const      { return NAME; }
str KatinaPluginTeamBalancer::get_version() const   { return VERSION; }


bool KatinaPluginTeamBalancer::open()
{
    statsPlugin = dynamic_cast<KatinaPluginStats*>( katina.get_plugin("katina::stats", "0.1-dev") );
    
    // Register for events
	katina.add_log_event(this, EXIT);
    //katina.add_log_event(this, CLIENT_CONNECT);
    katina.add_log_event(this, CLIENT_BEGIN);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, SAY);
    
    return true;
}


void KatinaPluginTeamBalancer::close()
{
    
}



bool KatinaPluginTeamBalancer::init_game(siz min, siz sec, const str_map& cvars)
{
    rateAllPlayers();
    buildTeams(teamBuilderInit);
    
    return true;
}


bool KatinaPluginTeamBalancer::client_begin(siz min, siz sec, siz client)
{
    float rating = ratePlayer(client);
    
    soss oss;
    oss << "Rating for '" << katina.getPlayerName(client) << "': " << rating;
    rcon.chat(oss.str());
    
    return true;
}


bool KatinaPluginTeamBalancer::client_disconnect(siz min, siz sec, siz client)
{
    playerRatings.erase(client);
    buildTeams(teamBuilderLeave);
    
    return true;
}


bool KatinaPluginTeamBalancer::ctf(siz min, siz sec, siz client, siz team, siz act)
{
    if(act == 1) // Capture
        buildTeams(teamBuilderCapture);
    
    return true;
}


bool KatinaPluginTeamBalancer::exit(siz min, siz sec)
{
    if(statsPlugin != NULL)
        lastStats.push_back(statsPlugin->stats);
    
    if(lastStats.size() > numLastStats)
        lastStats.erase(lastStats.begin());
}


bool KatinaPluginTeamBalancer::say(siz min, siz sec, const GUID& guid, const str& text)
{
    siz client = katina.getClientNr(guid);
	siss iss(text);
    str cmd;
    
	if(!(iss >> cmd))
		return true;

	if(cmd == "!rating")
	{
        ratePlayer(client);
        
        soss oss;
        oss << "Rating for '" << katina.getPlayerName(client) << "': " << playerRatings[client];
        rcon.chat(oss.str());
	}
    
    else if(cmd == "!ratings")
    {
        rateAllPlayers();
        
        for(siz_guid_map_citer it = katina.clients.begin(); it != katina.clients.end(); ++it)
        {
            soss oss;
            oss << "Rating for '" << katina.getPlayerName(it->first) << "': " << playerRatings[it->first];
            rcon.chat(oss.str());
        }
    }
    
    else if(cmd == "!teams")
    {
        rateAllPlayers();
        buildTeams(teamBuilderInit);
    }
    
    return true;
}


} } // Namespace katina::plugin



