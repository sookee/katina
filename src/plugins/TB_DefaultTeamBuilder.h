#pragma once

#include "KatinaPluginTeamBalancer.h"

#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>


namespace katina { namespace plugin {
    
    
    
class DefaultTeamBuilder : public TeamBuilder
{
private:
    bool is1vs1() const;
    
    static unsigned int nextTeamMask(unsigned int x);
    static str bitstring(unsigned int n, siz gap);
    
    float absDifference(siz teamMask, std::vector<float>& ratings) const;
    

public:
    DefaultTeamBuilder(Katina& katina, KatinaPluginTeamBalancer& plugin) : TeamBuilder(katina, plugin) {}
    virtual bool buildTeams(siz_float_map playerRatings, siz_map& destTeams);
};



class MinimalChangesTeamBuilder : public TeamBuilder
{
public:
    MinimalChangesTeamBuilder(Katina& katina, KatinaPluginTeamBalancer& plugin) : TeamBuilder(katina, plugin) {}
    virtual bool buildTeams(siz_float_map playerRatings, siz_map& destTeams) { return false; }
};
    
    
    
} } // Namespace katina::plugin