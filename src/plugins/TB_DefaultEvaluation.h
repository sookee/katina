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
    
    void loadStats(GUID guid, stat& dest);
    
    static siz getNumFrags(const stat& s);
    static siz getNumDeaths(const stat& s);
    
    static std::pair<siz, float> getShotsHits(const stat& s);
    
    
public:
    DefaultEvaluation(Katina& katina, KatinaPluginTeamBalancer& plugin);
    virtual float calcRating(slot client);
};



} } // Namespace katina::plugin
