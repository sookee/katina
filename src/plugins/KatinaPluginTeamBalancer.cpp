#include "KatinaPluginTeamBalancer.h"
#include "TB_DefaultEvaluation.h"
#include "TB_DefaultTeamBuilder.h"

//#include <katina/utils.h>


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
    minTimeBetweenRebalancing(20),
    scoreRed(0),
    scoreBlue(0)
{
    evaluation          = new DefaultEvaluation(katina, *this);
    
    teamBuilderInit     = new DefaultTeamBuilder(katina, *this);
    teamBuilderJoin     = new OnJoinTeamBuilder(katina, *this);
    teamBuilderLeave    = new MinimalChangesTeamBuilder(katina, *this);
    teamBuilderCapture  = new MinimalChangesTeamBuilder(katina, *this);
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


void KatinaPluginTeamBalancer::buildTeams(TeamBuilder* teamBuilder, TeamBuilderEvent event, void* payload, bool skipTimeCheck)
{
    if(teamBuilder == NULL)
        return;
    
    if(!skipTimeCheck && lastBalancing > katina.now - minTimeBetweenRebalancing)
        return;

    // TODO:
    str botSkill;
    //rcon.command("g_spskill", botSkill);
    //rcon.chat("BotSkill Reply: " + botSkill);
    botSkill = "2";
    
    // Call team building algorithm
    siz_map teams;
    if(!teamBuilder->buildTeams(playerRatings, teams, event, payload))
        return;
    
    // Switch all players via rcon
    siz switched = 0;
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
            
            QueuedChange qc;
            qc.timestamp = katina.now;
            qc.targetTeam = it->second;
            queuedChanges[it->first] = qc;
            
            bug("Queued change: client: " << it->first << " target team: " << it->second);
        }
        
        ++switched;
    }

    if(switched > 0)
    {
        lastBalancing = katina.now;
        rcon.cp("Teams adjusted");
    }
    
    printTeams(true, false);
}

siz nameLength(str name)
{
    siz len = name.length();
    for(int i=0; i<name.length()-1; ++i)
    {
        if(name.at(i) == '^' && name.at(i+1) >= '0' && name.at(i+1) <= '9')
            len -= 2;
    }
    
    return len;
}


void KatinaPluginTeamBalancer::printTeams(bool printBug, bool printChat)
{
    rateAllPlayers();
    
    siz col1Len = 11;
    siz col2Len = 12;
    static siz pad = 7;
    
    for(guid_str_map_citer it = katina.players.begin(); it != katina.players.end(); ++it)
    {
        siz len = nameLength(it->second) + 8;
        
        if(katina.teams[it->first] == TEAM_R && col1Len < len)
            col1Len = len;
        else if(katina.teams[it->first] == TEAM_B && col2Len < len)
            col2Len = len;
    }
    
    typedef std::map<float, siz> sorted;
    sorted red;
    sorted blue;
    
    for(siz_float_map_citer it = playerRatings.begin(); it != playerRatings.end(); ++it)
    {
        if(katina.getTeam(it->first) == TEAM_R)
            red[it->second] = it->first;
        else if(katina.getTeam(it->first) == TEAM_B)
            blue[it->second] = it->first;
    }
    
    
    {
        soss oss;
        oss << "^1=== RED ===^7";
        for(int i=11; i<col1Len+pad; ++i)
            oss << ".";
        oss << "^4=== BLUE ===";
        if(printChat) katina.server.chat(oss.str());
        if(printBug) bug(oss.str());
    }
    
    
    sorted::reverse_iterator itRed  = red.rbegin();
    sorted::reverse_iterator itBlue = blue.rbegin();
    float sumRed = 0.0f;
    float sumBlue = 0.0f;
    
    while(itRed != red.rend() || itBlue != blue.rend())
    {
        soss oss;
        
        if(itRed != red.rend())
        {
            siz rating = round(itRed->first);
            str name = katina.players[katina.clients[itRed->second]];
            oss << name;
            
            for(int i=nameLength(name); i<col1Len-8; ++i)
                oss << ".";
            
            oss << " ^3(^7";
            if(rating < 10000) oss << " ";
            if(rating < 1000) oss << ".";
            if(rating < 100) oss << ".";
            if(rating < 10) oss << ".";
            oss << "^1" << rating << "^3)^7";
            
            for(int i=0; i<pad; ++i)
                oss << ".";
            
            ++itRed;
            sumRed += rating;
        }
        else
        {
            for(int i=0; i<col1Len+pad; ++i)
                oss << ".";
        }
        
        if(itBlue != blue.rend())
        {
            siz rating = round(itBlue->first);
            str name = katina.players[katina.clients[itBlue->second]];
            oss << name;
            
            for(int i=nameLength(name); i<col2Len-8; ++i)
                oss << ".";
                    
            oss << " ^3(^7";
            if(rating < 10000) oss << " ";
            if(rating < 1000) oss << ".";
            if(rating < 100) oss << ".";
            if(rating < 10) oss << ".";
            oss << "^4" << rating << "^3)";
            
            ++itBlue;
            sumBlue += rating;
        }

        if(printChat) katina.server.chat(oss.str());
        if(printBug) bug(oss.str());
    }
    
    soss oss;
    oss << "Sum: ^1" << sumRed << " ^7/ ^4" << sumBlue << " ^7- Score: ^1" << scoreRed << " ^7/ ^4" << scoreBlue;
    if(printChat) katina.server.chat(oss.str());
    if(printBug) bug(oss.str());
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
    //katina.add_log_event(this, CLIENT_BEGIN);
	katina.add_log_event(this, CLIENT_DISCONNECT);
    katina.add_log_event(this, CLIENT_SWITCH_TEAM);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, SAY);
    katina.add_log_event(this, HEARTBEAT);
    
    return true;
}


void KatinaPluginTeamBalancer::close()
{
    
}



bool KatinaPluginTeamBalancer::init_game(siz min, siz sec, const str_map& cvars)
{
    scoreRed = 0;
    scoreBlue = 0;
    teamScoreHistory.clear();
    
    rateAllPlayers();
    buildTeams(teamBuilderInit, TB_INIT, NULL, true);
    
    return true;
}


bool KatinaPluginTeamBalancer::client_switch_team(siz min, siz sec, siz num, siz teamBefore, siz teamNow)
{
    // Skip if it was a queued change
    queued_changes_map::iterator it = queuedChanges.find(num);
    if(it != queuedChanges.end())
    {
        if(it->second.targetTeam == teamNow)
        {
            queuedChanges.erase(it);
            bug("ACCEPTED QUEUED CHANGE");
        }
        else
            bug("====> CLIENT NOT IN EXPECTED TARGET TEAM!!");
        
        return false;
    }
    
    // Player joined game / switched teams
    if(teamNow == TEAM_R || teamNow == TEAM_B)
    {
        rateAllPlayers();
        
        TB_JoinData payload;
        payload.client     = num;
        payload.teamBefore = teamBefore;
        payload.teamNow    = teamNow;
        
        buildTeams(teamBuilderJoin, TB_JOIN, &payload, true);
    }
    
    // Player joined spec
    else if(teamNow == TEAM_S && (teamBefore == TEAM_R || teamBefore == TEAM_B))
    {
        rateAllPlayers();
        buildTeams(teamBuilderLeave, TB_DISCONNECT, NULL, true);
    }
    
    // Player just joined
    else if(teamBefore == TEAM_U)
    {
        float rating = ratePlayer(num);

        soss oss;
        oss << "Rating for " << katina.getPlayerName(num) << ": " << rating;
        rcon.chat(oss.str());
    }
    
    return true;
}


bool KatinaPluginTeamBalancer::client_disconnect(siz min, siz sec, siz client)
{
    playerRatings.erase(client);
    rateAllPlayers();
    buildTeams(teamBuilderLeave, TB_DISCONNECT, NULL, true);
    
    return true;
}


bool KatinaPluginTeamBalancer::ctf(siz min, siz sec, siz client, siz team, siz act)
{
    // Count flag captures (team score)
    if(act == FL_CAPTURED)
    {
        teamScoreHistory.push_back(team);
        
        // Dunno why, but those flags are inverted here
        if(team == TEAM_B)
            ++scoreRed;
        else if(team == TEAM_R)
            ++scoreBlue;
    
        siz capturelimit = to<siz>(katina.cvars["capturelimit"]);
        
        if(scoreRed < capturelimit && scoreBlue < capturelimit)
        {
            rateAllPlayers();
            buildTeams(teamBuilderCapture, TB_CAPTURE, NULL);
        }
    }
    
    return true;
}


bool KatinaPluginTeamBalancer::exit(siz min, siz sec)
{
    if(statsPlugin != NULL)
        lastStats.push_back(statsPlugin->stats);
    
    if(lastStats.size() > numLastStats)
        lastStats.erase(lastStats.begin());
    
    printTeams(true);
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
    
    /*else if(cmd == "!ratings")
    {
        rateAllPlayers();
        
        for(siz_guid_map_citer it = katina.clients.begin(); it != katina.clients.end(); ++it)
        {
            soss oss;
            oss << "Rating for '" << katina.getPlayerName(it->first) << "': " << playerRatings[it->first];
            rcon.chat(oss.str());
        }
    }*/
    
    /*else if(cmd == "!teams")
    {
        rateAllPlayers();
        buildTeams(teamBuilderInit, TB_COMMAND, NULL);
    }*/
    
    else if(cmd == "!teamrating")
    {
        printTeams();
    }
    
    return true;
}



void KatinaPluginTeamBalancer::heartbeat(siz min, siz sec)
{
    static siz waitTime = 1;
    static std::vector<siz> toDelete;
    
    // Resend queued changes that didn't complete (UDP..)
    for(queued_changes_map::iterator it = queuedChanges.begin(); it != queuedChanges.end(); ++it)
    {
        if(katina.now - it->second.timestamp >= waitTime)
        {
            // Check if player somehow already is in target team
            if(katina.teams[katina.clients[it->first]] == it->second.targetTeam)
            {
                toDelete.push_back(it->first);
                continue;
            }
            
            soss oss;
            oss << "!putteam " << it->first << " " << TEAM_CHAR[it->second.targetTeam];
            rcon.command(oss.str());
            
            it->second.timestamp = katina.now;
            
            bug("====> requeued client: " << it->first << " target team: " << it->second.targetTeam);
        }
    }
    
    for(siz i=0; i<toDelete.size(); ++i)
        queuedChanges.erase(toDelete[i]);
    
    toDelete.clear();
}



bool TeamBuilder::is1vs1() const
{
    siz r = 0;
    siz b = 0;
    
    for(guid_siz_map_citer it = katina.teams.begin(); it != katina.teams.end(); ++it)
    {
        if(it->second == TEAM_R)
            ++r;
        else if(it->second == TEAM_B)
            ++b;
    }
    
    return r == 1 && b == 1;
}



} } // Namespace katina::plugin



