#include "TB_DefaultEvaluation.h"


namespace katina { namespace plugin {

    
    
DefaultEvaluation::DefaultEvaluation(Katina& katina, KatinaPluginTeamBalancer& plugin) :
    PlayerEvaluation(katina, plugin)
{ 
    db.config(
        katina.get("db.host", "localhost"),
        katina.get("db.port", 3306),
        katina.get("db.user"),
        katina.get("db.pass", ""),
        katina.get("db.base"));
}



void DefaultEvaluation::loadStats(GUID guid, stats& dest)
{
    db.on();
    
    str_vec_vec rows;
    db.select("SELECT weap, SUM(count) FROM kills WHERE guid='" + str(guid) + "' GROUP BY weap", rows, 2);
    for(siz i=0; i<rows.size(); ++i)
        dest.kills[ to<siz>(rows[i][0]) ] = to<siz>(rows[i][1]);
    
    rows.clear();
    db.select("SELECT weap, SUM(count) FROM deaths WHERE guid='" + str(guid) + "' GROUP BY weap", rows, 2);
    for(siz i=0; i<rows.size(); ++i)
        dest.deaths[ to<siz>(rows[i][0]) ] = to<siz>(rows[i][1]);
    
    rows.clear();
    db.select("SELECT SUM(count) FROM caps WHERE guid='" + str(guid) + "'", rows, 1);
    if(rows.size() && rows[0][0].length())
        dest.flags[FL_CAPTURED] = to<siz>(rows[0][0]);
    
    rows.clear();
    db.select("SELECT SUM(count) FROM time WHERE guid='" + str(guid) + "'", rows, 1);
    if(rows.size() && rows[0][0].length())
        dest.logged_time = to<siz>(rows[0][0]);
    
    rows.clear();
    db.select("SELECT weap, SUM(shots) FROM weapon_usage WHERE guid='" + str(guid) + "' GROUP BY weap", rows, 2);
    for(siz i=0; i<rows.size(); ++i)
        dest.weapon_usage[ to<siz>(rows[i][0]) ] = to<siz>(rows[i][1]);
    
    rows.clear();
    db.select("SELECT `mod`, SUM(weightedHits) FROM damage WHERE guid='" + str(guid) + "' GROUP BY `mod`", rows, 2);
    for(siz i=0; i<rows.size(); ++i)
        dest.mod_damage[ to<siz>(rows[i][0]) ].weightedHits = to<float>(rows[i][1]);
    
    //db.off();
}
    
    
    
siz DefaultEvaluation::getNumFrags(stats s)
{
    siz frags = 0;
    for(siz_map_citer it = s.kills.begin(); it != s.kills.end(); ++it)
        frags += it->second;
    
    return frags;
}



siz DefaultEvaluation::getNumDeaths(stats s)
{
    siz deaths = 0;
    for(siz_map_citer it = s.deaths.begin(); it != s.deaths.end(); ++it)
        deaths += it->second;
    
    return deaths;
}


    
std::pair<siz, float> DefaultEvaluation::getShotsHits(stats s)
{
    siz shots = 0;
    float hits  = 0;
    
    for(siz_map_citer it = s.weapon_usage.begin(); it != s.weapon_usage.end(); ++it)
        shots += it->second;
    
    for(moddmg_map_citer it = s.mod_damage.begin(); it != s.mod_damage.end(); ++it)
        hits += it->second.weightedHits;
    
    return std::pair<siz, float>(shots, hits);
}
    
    
    
float DefaultEvaluation::calcRating(siz client)
{
    KatinaPluginStats* statsPlugin = balancerPlugin.getStatsPlugin();
    if(statsPlugin == NULL)
        return 1000.0f;
    
    GUID guid = katina.clients[client];
    //if(guid.is_bot())
    //    return 1000.0f;
    
    // Sum up values of multiple games
    stats& sref = statsPlugin->stats[guid];
    
    siz frags  = getNumFrags(sref);
    siz caps   = sref.flags[FL_CAPTURED];
    siz deaths = getNumDeaths(sref);
    siz time   = sref.logged_time;
    
    std::pair<siz, float> values = getShotsHits(sref);
    siz shots  = values.first;
    float hits = values.second;
    
    
    /*stat_list& lastStats = balancerPlugin.getLastStats();
    for(stat_list_citer it = lastStats.begin(); it != lastStats.end(); ++it)
    {
        stats s = (*it).at(guid);
        
        // FC / DT
        frags  += getNumFrags(s);
        caps   += s.flags[FL_CAPTURED];
        deaths += getNumDeaths(s);
        time   += s.logged_time;
        
        // Accuracy
        values  = getShotsHits(s);
        shots  += values.first;
        hits   += values.second;
    }*/
    
    
    // Add values from DB
    stats s;
    loadStats(guid, s);
    
    frags  += getNumFrags(s);
    caps   += s.flags[FL_CAPTURED];
    deaths += getNumDeaths(s);
    time   += s.logged_time;
    
    values  = getShotsHits(s);
    shots  += values.first;
    hits   += values.second;
    
    // Calculate rating
    if(time < 60)
        return 1000.0f;
    
    if(frags == 0)
        frags = 1;
    if(deaths == 0)
        deaths = 1;
    
    float f = frags + (caps * 8.0f);
    float t = time / 3600.0f;
    
    float rating = sqrt( (f / t) * sqrt(f / deaths) );
    
    float accuracy = hits / shots;
    if(shots == 0)
        accuracy = 0.0f;
    
    return round(rating * 100.0f * (accuracy+1.0));
}



} } // Namespace katina::plugin


