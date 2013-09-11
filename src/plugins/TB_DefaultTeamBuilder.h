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
    //static unsigned int nextTeamMask(unsigned int x);
    //static str bitstring(unsigned int n, siz gap);
    
    //float absDifference(siz teamMask, const std::vector<float>& ratings) const;
    

public:
    DefaultTeamBuilder(Katina& katina, KatinaPluginTeamBalancer& plugin) : TeamBuilder(katina, plugin) {}
    virtual bool buildTeams(siz_float_map playerRatings, siz_map& destTeams, TeamBuilderEvent event=TB_UNKNOWN, void* payload=NULL);
};



class MinimalChangesTeamBuilder : public TeamBuilder
{
private:
    struct TeamProposal
    {
        siz teamMask;
        float ratingDiff;
        
        TeamProposal() : teamMask(0), ratingDiff(0.0f) {}
        TeamProposal(siz teamMask, float ratingDiff) : teamMask(teamMask), ratingDiff(ratingDiff) {}
    };
    
    typedef std::map<siz, TeamProposal> proposal_map; // sorted by key "num changes"
    typedef proposal_map::iterator proposal_map_iter;
    typedef proposal_map::const_iterator proposal_map_citer;

    siz currentTeamMask(const std::vector<siz>& players) const;
    siz numChangesNeeded(siz teamMask, const std::vector<siz>& players) const;
    void insertTeamMask(proposal_map& proposals, siz teamMask, float diff, const std::vector<siz>& players) const;
    
    
public:
    MinimalChangesTeamBuilder(Katina& katina, KatinaPluginTeamBalancer& plugin) : TeamBuilder(katina, plugin) {}
    virtual bool buildTeams(siz_float_map playerRatings, siz_map& destTeams, TeamBuilderEvent event=TB_UNKNOWN, void* payload=NULL);
};



class OnJoinTeamBuilder : public TeamBuilder
{
private:
    
public:
    OnJoinTeamBuilder(Katina& katina, KatinaPluginTeamBalancer& plugin) : TeamBuilder(katina, plugin) {}
    virtual bool buildTeams(siz_float_map playerRatings, siz_map& destTeams, TeamBuilderEvent event=TB_UNKNOWN, void* payload=NULL);
};


    
} } // Namespace katina::plugin