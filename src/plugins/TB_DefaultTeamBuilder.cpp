#include "TB_DefaultTeamBuilder.h"


namespace katina { namespace plugin {


    
///////////////////////////////////////////////////////////////////////////////////////////////////
// DefaultTeamBuilder
///////////////////////////////////////////////////////////////////////////////////////////////////

// http://stackoverflow.com/questions/506807/creating-multiple-numbers-with-certain-number-of-bits-set
unsigned int nextTeamMask(unsigned int x)
{
    if(x == 0)
        return 0;

    unsigned int smallest     = (x & -x);
    unsigned int ripple       = x + smallest;
    unsigned int new_smallest = (ripple & -ripple);
    unsigned int ones         = ((new_smallest/smallest) >> 1) - 1;
    
    return ripple | ones;
}



str bitstring(unsigned int n, siz stop=32)
{
    str bits("");
    
    for(siz i=0; i<stop; ++i)
    {
        unsigned int check = 1 << i;
        bits = ((n & check) ? "1" : "0") + bits;
    }
    
    return bits;
}



float difference(siz teamMask, const std::vector<float>& ratings, siz scoreR=0, siz scoreB=0)
{
    float r = 0.0f;
    float b = 0.0f;
    siz countR = 0;
    siz countB = 0;
    
    for(siz i=0; i<ratings.size(); ++i, teamMask >>= 1)
    {
        if(teamMask & 1)
        {
            b += ratings[i];
            ++countB;
        }
        else
        {
            r += ratings[i];
            ++countR;
        }
    }
    
    if(countR + countB >= 5)
    {
        if(countR > countB)
        {
            float bonus = (float) countR / countB;
            bonus = (bonus - 1.0f) / 5.0f;
            r *= (bonus + 1.0f);
        }
        else if(countB > countR)
        {
            float bonus = (float) countB / countR;
            bonus = (bonus - 1.0f) / 5.0f;
            b *= (bonus + 1.0f);
        }
    }
    
    scoreR += 2;
    scoreB += 2;
    
    if(scoreR > scoreB)
    {
        float bonus = (float) scoreR / scoreB;
        bonus = (bonus - 1.0f) / 20.0f;
        r *= (bonus + 1.0f);
    }
    else if(scoreB > scoreR)
    {
        float bonus = (float) scoreB / scoreR;
        bonus = (bonus - 1.0f) / 20.0f;
        b *= (bonus + 1.0f);
    }
    
    return r - b;
}



// Builds teams from scratch with the minimal difference between the sum of ratings.
// This is known as the 'partition problem' which is NP-complete.
// We're going to find the best solution with a brute force approach.
// http://www.americanscientist.org/issues/issue.aspx?id=3278&y=0&no=&content=true&page=4&css=print
bool DefaultTeamBuilder::buildTeams(slot_float_map playerRatings, slot_siz_map& destTeams, TeamBuilderEvent event, void* payload)
{
    if(katina.getClients().size() < 3 || is1vs1())
        return false;
    
    // Create list of active players in the game
    std::vector<slot> ingamePlayers;
    std::vector<float> ratings;
    
	slot client;
	for(guid_siz_map_citer it = katina.getTeams().begin(); it != katina.getTeams().end(); ++it)
	{
    	if(it->second != TEAM_R && it->second != TEAM_B)
        	continue;

    	if((client = katina.getClientSlot(it->first)) == bad_slot)
    		continue;

    	ingamePlayers.push_back(client);
    	ratings.push_back(playerRatings[client]);
	}
    
    if(ingamePlayers.size() < 3)
        return false;
    
    // Search for teams with minimal rating difference
    siz teamMask   = pow(2, ingamePlayers.size() / 2) - 1;
    siz end        = 1 << ingamePlayers.size();
    siz bestTeam   = teamMask;
    float bestDiff = 100000000000.0f; // TODO: Use positive infinity instead

    do
    {
        float diff = abs(difference(teamMask, ratings));
        if(diff < bestDiff)
        {
            bestTeam = teamMask;
            bestDiff = diff;
        }
        
       teamMask = nextTeamMask(teamMask);
    } while(!(teamMask & end));

    // Build map with team definitions
    for(int i=0; i<ingamePlayers.size(); ++i, bestTeam >>= 1)
        destTeams[ ingamePlayers[i] ] = (bestTeam & 1) ? TEAM_B : TEAM_R;
    
    return true;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
// MinimalChangesTeamBuilder
///////////////////////////////////////////////////////////////////////////////////////////////////

siz MinimalChangesTeamBuilder::currentTeamMask(const std::vector<slot>& players) const
{
    siz teamMask = 0;
    for(siz i=0; i<players.size(); ++i)
    {
        if(katina.getTeam(players[i]) == TEAM_B)
            teamMask |= (1 << i);
    }
    
    return teamMask;
}



siz MinimalChangesTeamBuilder::numChangesNeeded(siz teamMask, const std::vector<slot>& players) const
{
    siz changes = 0;
    
    for(siz i=0; i<players.size(); ++i, teamMask >>= 1)
    {
        if(teamMask & 1)
        {
            if(katina.getTeam(players[i]) == TEAM_R)
                ++changes;
        }
        else if(katina.getTeam(players[i]) == TEAM_B)
            ++changes;
    }
    
    return changes;
}



void MinimalChangesTeamBuilder::insertTeamMask(proposal_map& proposals, siz teamMask, float diff, const std::vector<slot>& players) const
{
    siz changesNeeded = numChangesNeeded(teamMask, players);
    
    proposal_map_citer it = proposals.find(changesNeeded);
    if(it == proposals.end() || (it != proposals.end() && it->second.ratingDiff > diff))
        proposals[changesNeeded] = TeamProposal(teamMask, diff);
}



bool MinimalChangesTeamBuilder::buildTeams(slot_float_map playerRatings, slot_siz_map& destTeams, TeamBuilderEvent event, void* payload)
{
    if(katina.getClients().size() < 3 || is1vs1())
        return false;
    
    KatinaPluginStats* statsPlugin = balancerPlugin.getStatsPlugin();
    if(statsPlugin == NULL)
        return false;
    
    // Create list of active players in the game
    std::vector<slot> ingamePlayers;
    std::vector<float> ratings;
    
	slot client;
	for(guid_siz_map_citer it = katina.getTeams().begin(); it != katina.getTeams().end(); ++it)
	{
    	if(it->second != TEAM_R && it->second != TEAM_B)
        	continue;

    	if((client = katina.getClientSlot(it->first)) == bad_slot)
    		continue;

    	ingamePlayers.push_back(client);
    	ratings.push_back(playerRatings[client]);
	}

	if(ingamePlayers.size() < 3)
        return false;
    
    bug("building teams for " << ingamePlayers.size() << " players");
    
    // Calculate current team difference
    siz currentTeam = currentTeamMask(ingamePlayers);
    const float currentDiff = abs( difference(currentTeam, ratings) );
    
    // Evaluate all valid team combinations and sort proposed teams
    //siz teamMask = balancerPlugin.getScoreRed() > balancerPlugin.getScoreBlue() ? (ingamePlayers.size() / 2.0f) + 0.5f : ingamePlayers.size() / 2;
    siz teamMask = ingamePlayers.size() / 2;
    teamMask = pow(2, teamMask) - 1;
    siz end = 1 << ingamePlayers.size();
    proposal_map proposals;

    do
    {
        float diff = abs(difference(teamMask, ratings));
        
        insertTeamMask(proposals, teamMask, diff, ingamePlayers);

        // Invert teams
        // We only need to do this if teams can't be even (odd player count)
        // Otherwise the inverted mask will be generated anyway
        if((ingamePlayers.size() & 1)) // && balancerPlugin.getScoreRed() == balancerPlugin.getScoreBlue())
            insertTeamMask(proposals, ~teamMask, diff, ingamePlayers);
        
       teamMask = nextTeamMask(teamMask);
    } while(!(teamMask & end));
    
    // Search for best teamMask with minimal number of changes to current teams
    if(proposals.empty())
    {
        bug("no proposals available");
        return false;
    }

    float maxImprove = -1000000000000.0f;
    for(proposal_map_citer it=proposals.begin(); it != proposals.end(); ++it)
    {
        float improve = (currentDiff - it->second.ratingDiff) / sqrt(it->first+1.0f);
        
        bug("proposal with " << it->first << " changes: " << bitstring(it->second.teamMask, ingamePlayers.size()) << " diff: " << it->second.ratingDiff << " improvement: " << improve);
        
        if(improve > maxImprove)
        {
            teamMask = it->second.teamMask;
            maxImprove = improve;
        }
    }
    
    // The chosen mask is the same as the current one, diff delta = 0
    if(teamMask & end)
    {
        bug("no proposal chosen");
        return false;
    }
    
    bug("choosed proposal: " << bitstring(teamMask, ingamePlayers.size()) << " improvement: " << maxImprove);
    
    // Build map with team definitions
    for(int i=0; i<ingamePlayers.size(); ++i, teamMask >>= 1)
        destTeams[ ingamePlayers[i] ] = (teamMask & 1) ? TEAM_B : TEAM_R;
    
    return true;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// OnJoinTeamBuilder
///////////////////////////////////////////////////////////////////////////////////////////////////

bool OnJoinTeamBuilder::buildTeams(slot_float_map playerRatings, slot_siz_map& destTeams, TeamBuilderEvent event, void* payload)
{
    if(katina.getClients().size() < 3 || is1vs1())
        return false;
    
    if(event != TB_JOIN || payload == NULL)
        return false;
    
    TB_JoinData& data = *((TB_JoinData*)payload);
    
    // Count players in teams
    siz numRed  = 0;
    siz numBlue = 0;
    for(guid_siz_map_citer it = katina.getTeams().begin(); it != katina.getTeams().end(); ++it)
    {
        if(it->second == TEAM_R)
            ++numRed;
        else if(it->second == TEAM_B)
            ++numBlue;
    }
    
    // If teams are even now: nothing to do
    if(numRed == numBlue)
        return false;
    
    str playerName = katina.getPlayerName(data.client);
    
    // Player joined the game from spectators
    //if(data.teamBefore == TEAM_S)
    {
        // Check if the player joined the team that already had more players
        if(data.teamNow == TEAM_R && numRed > (numBlue+1))
        {
            // Switch player to blue
            destTeams[data.client] = TEAM_B;
            katina.server.chat("^1Bad Join, ^7" + playerName + "^1!");
            return true;
        }
        else if(data.teamNow == TEAM_B && numBlue > (numRed+1))
        {
            // Switch player to red
            destTeams[data.client] = TEAM_R;
            katina.server.chat("^1Bad Join, ^7" + playerName + "^1!");
            return true;
        }
    }
    
    // Check if the player joined the winning team
    if(data.teamNow == TEAM_R && numRed > numBlue && balancerPlugin.getScoreRed() > balancerPlugin.getScoreBlue())
    {
        // Switch player to blue
        destTeams[data.client] = TEAM_B;
        katina.server.chat("^1Don't be a lame winning team joiner, ^7" + playerName + "^1!");
        return true;
    }
    else if(data.teamNow == TEAM_B && numBlue > numRed && balancerPlugin.getScoreBlue() > balancerPlugin.getScoreRed())
    {
        // Switch player to red
        destTeams[data.client] = TEAM_R;
        katina.server.chat("^1Don't be a lame winning team joiner, ^7" + playerName + "^1!");
        return true;
    }
    
    return false;
}


} } // Namespace katina::plugin


