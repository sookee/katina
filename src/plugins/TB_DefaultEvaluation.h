#pragma once

#include "KatinaPluginTeamBalancer.h"

#include <katina/Database.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>


namespace katina { namespace plugin {

using namespace katina::data;
    
class DefaultEvaluation : public PlayerEvaluation
{
private:
    Database db;
    
    void loadStats(GUID guid, stats& dest);
    
    static siz getNumFrags(stats s);
    static siz getNumDeaths(stats s);
    
    static std::pair<siz, float> getShotsHits(stats s);
    
    
public:
    DefaultEvaluation(Katina& katina, KatinaPluginTeamBalancer& plugin);
    virtual float calcRating(slot client);
};



} } // Namespace katina::plugin
