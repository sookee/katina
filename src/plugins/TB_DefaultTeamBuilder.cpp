#include "TB_DefaultTeamBuilder.h"


namespace katina { namespace plugin {

    

bool DefaultTeamBuilder::is1vs1() const
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



// http://stackoverflow.com/questions/506807/creating-multiple-numbers-with-certain-number-of-bits-set
unsigned int DefaultTeamBuilder::nextTeamMask(unsigned int x)
{
    unsigned int smallest, ripple, new_smallest, ones;

    if(x == 0)
        return 0;

    smallest     = (x & -x);
    ripple       = x + smallest;
    new_smallest = (ripple & -ripple);
    ones         = ((new_smallest/smallest) >> 1) - 1;
    return ripple | ones;
}



str DefaultTeamBuilder::bitstring(unsigned int n, siz gap)
{
    str bits("");
    --gap;
    
    for(siz i=0; i<32; ++i)
    {
        unsigned int check = 1 << i;
        bits = ((n & check) ? "1" : "0") + bits;
        
        if(i == gap)
            bits = " " + bits;
    }
    
    return bits;
}



float DefaultTeamBuilder::absDifference(siz teamMask, std::vector<float>& ratings) const
{
    float r = 0;
    float b = 0;
    
    for(int i=0; i<ratings.size(); ++i, teamMask >>= 1)
    {
        if(teamMask & 1)
            b += ratings[i];
        else
            r += ratings[i];
    }
    
    return r > b ? r - b : b - r;
}



// Creates totally new teams
// This is known as the 'partition problem' which is NP-complete.
// We're going to find the best solution with a brute force approach.
// http://www.americanscientist.org/issues/issue.aspx?id=3278&y=0&no=&content=true&page=4&css=print
bool DefaultTeamBuilder::buildTeams(siz_float_map playerRatings, siz_map& destTeams)
{
    if(katina.clients.size() < 3)
        return false;
    
    if(is1vs1())
        return false;
    
    // Create list of active and spectating players in the game
    std::vector<siz> ingamePlayers;
    std::vector<float> ratings;
    std::vector<siz> specs;
    for(guid_siz_map_citer it = katina.teams.begin(); it != katina.teams.end(); ++it)
    {
        if(it->second == TEAM_R || it->second == TEAM_B)
        {
            siz client = katina.getClientNr(it->first);
            ingamePlayers.push_back(client);
            ratings.push_back( balancerPlugin.getRating(client) );
        }
        else
            specs.push_back(it->first);
    }
    
    // Search for teams with minimal rating difference
    siz teamMask   = pow(2, ingamePlayers.size() / 2) - 1;
    siz end        = 1 << ingamePlayers.size();
    siz bestTeam   = teamMask;
    float bestDiff = 100000000.0f;

    do
    {
        float diff = absDifference(teamMask, ratings);
        if(diff < bestDiff)
        {
            bestTeam = teamMask;
            bestDiff = diff;
        }
        
       teamMask = nextTeamMask(teamMask);
    } while(!(teamMask & end));

    // Build map with team definitions
    destTeams.clear();
    for(int i=0; i<ingamePlayers.size(); ++i, bestTeam >>= 1)
        destTeams[ ingamePlayers[i] ] = (bestTeam & 1) ? TEAM_B : TEAM_R;
    
    for(int i=0; i<specs.size(); ++i)
        destTeams[ specs[i] ] = TEAM_S;
    
    return true;
}
        


} } // Namespace katina::plugin


