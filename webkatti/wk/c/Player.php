<?php

namespace wk\c;

use M;
use Storage;



class Player
{

    static function get($guid)
    {
        $c = new Layout();
        $c->setView(__METHOD__);

        $c->name = M::user()->getNameByGuid($guid, $c->registered);

        $c->players = M::player()->db()
            ->key($guid, 'guid')
            ->addOrderBy('date', true)
            ->all();

        $games = Storage::main()
            ->select('select distinct game.*
                from game natural join kills
                where guid=? and date > now() - interval 1 month
                order by game_id desc', $guid);

        $gameIds = '';
        foreach ($games as $game)
        {
            $gameIds .= $game['game_id'] . ',';
        }

        $c->games = Game::_list($games);

        if (!empty($gameIds))
        {
            $gameIds = substr($gameIds, 0, -1);


            /**
             * Opponents
             */

            $r = M::ovo()->db()
                ->fields('guid1, guid2, sum(count) as count')
                ->where("game_id in ($gameIds) and (guid1=? or guid2=?)", [$guid, $guid])
                ->groupBy('guid1, guid2')
                ->having('count > ?', M::settings()->get('min_deaths_ovo'))
                ->all();
            $ovos = [];
            foreach ($r as $row)
            {
                if ($row['guid1'] == $guid)
                {
                    $ovos[$row['guid2']]['kills'] = $row['count'];
                }
                else
                {
                    $ovos[$row['guid1']]['deaths'] = $row['count'];
                }
            }

            $c->ovos = [];
            foreach ($ovos as $ovo_guid => $ovo)
            {
                if (!empty($ovo['kills']) && !empty($ovo['deaths']))
                {
                    $c->ovos[$ovo_guid] = $ovo;
                }
            }
            unset($ovos);

            if (!empty($c->ovos))
            {
                M::user()->setNamesByGuid($c->ovos);

                foreach ($c->ovos as $ovo_guid => &$ovo)
                {
                    $ovo['kd'] = $ovo['kills'] / $ovo['deaths'];
                }
                unset($ovo);

                uasort($c->ovos, function($a, $b)
                {
                    return ($a['kd'] - $b['kd']) * 1000000;
                });

                foreach ($c->ovos as &$ovo)
                {
                    $ovo['kd'] = round($ovo['kd'] * 100);
                }
                unset($ovo);
            }



            /**
             * Weapons
             */

            $c->weapons = [];
            $c->weaponsTotal = [
                'killCount' => 0,
                'deathCount' => 0,
                'shots' => 0,
                'hits' => 0,
                'dmgDone' => 0,
                'hitsRecv' => 0,
                'dmgRecv' => 0
            ];

            
            // Kill count
            $r = M::kill()->db()
                ->fields('weap, sum(count) as count')
                ->where("game_id in ($gameIds) and guid=?", $guid)
                ->groupBy('weap')
                ->orderBy('count desc')
                ->allK();

            $sum = array_sum($r);
            foreach ($r as $weap => $count)
            {
                $c->weapons[$weap]['killsPercent'] = $count / $sum;
                $c->weapons[$weap]['killCount']    = $count;
                $c->weaponsTotal['killCount']     += $count;
            }

            
            // Death count
            $r = M::death()->db()
                ->fields('weap, sum(count) as count')
                ->where("game_id in ($gameIds) and guid=?", $guid)
                ->groupBy('weap')
                ->allK();

            $sum = array_sum($r);
            foreach ($r as $weap => $count)
            {
                $c->weapons[$weap]['deathsPercent'] = $count / $sum;
                $c->weapons[$weap]['deathCount']    = $count;
                $c->weaponsTotal['deathCount']     += $count;
            }
            
            
            // Shot count
            $r = M::weapon_usage()->db()
                ->fields('weap, sum(shots) as shots')
                ->where("guid=?", $guid)
                ->groupBy('weap')
                ->allK();
            
            foreach ($r as $weap => $shots)
            {
                foreach(\wk\Utils::$weapon_to_mod[$weap] as $mod)
                    $c->weapons[$mod]['shots'] = $shots;
                
                $c->weaponsTotal['shots'] += $shots;
            }
            
            
            // Damage
            $r = M::damage()->db()
                ->fields('`mod`, sum(hits) as hits, sum(dmgDone) as dmgDone, sum(hitsRecv) as hitsRecv, sum(dmgRecv) as dmgRecv')
                ->where("guid=?", $guid)
                ->groupBy('`mod`')
                ->all();
            
            foreach($r as $row)
            {
                $c->weapons[$row['mod']]['hits']     = $row['hits'];
                $c->weapons[$row['mod']]['hitsRecv'] = $row['hitsRecv'];
                $c->weapons[$row['mod']]['dmgDone']  = $row['dmgDone'];
                $c->weapons[$row['mod']]['dmgRecv']  = $row['dmgRecv'];
                $c->weapons[$row['mod']]['dmgRatio'] = @($row['dmgDone'] / $row['dmgRecv']);
                
                if(array_key_exists('shots', $c->weapons[$row['mod']]))
                    $c->weapons[$row['mod']]['accuracy'] = @($row['hits'] / $c->weapons[$row['mod']]['shots']);
                
                $c->weaponsTotal['hits']     += $row['hits'];
                $c->weaponsTotal['hitsRecv'] += $row['hitsRecv'];
                $c->weaponsTotal['dmgDone']  += $row['dmgDone'];
                $c->weaponsTotal['dmgRecv']  += $row['dmgRecv'];
            }
            
            foreach($c->weapons as $w)
            {
                $w['dmgDonePercent'] = @(@$w['dmgDone'] / $c->weaponsTotal['dmgDone']);
                $w['dmgRecvPercent'] = @(@$w['dmgRecv'] / $c->weaponsTotal['dmgRecv']);
            }
            
            $c->weaponsTotal['dmgRatio'] = @($c->weaponsTotal['dmgDone'] / $c->weaponsTotal['dmgRecv']);
            $c->weaponsTotal['accuracy'] = @($c->weaponsTotal['hits'] / $c->weaponsTotal['shots']);
            
            
            /**
             * Player stats
             */
            $c->stats = M::playerstats()->db()
                ->fields('sum(spawnKillsDone) as spawnKillsDone, sum(spawnKillsRecv) as spawnKillsRecv, sum(pushesDone) as pushesDone, '
                       . 'sum(pushesRecv) as pushesRecv, sum(healthPickedUp) as healthPickedUp, sum(armorPickedUp) as armorPickedUp')
                       //. 'sum(holyShitFrags) as holyShitFrags, sum(holyShitFragged) as holyShitFragged, '
                       //. 'sum(carrierFrags) as carrierFrags, sum(carrierFragsRecv) as carrierFragsRecv')
                ->where("guid=?", $guid)
                ->one();
        }

        $c->addTitle('Player');
        $c->addTitle(\wk\Utils::uncolor($c->name));

        return $c;
    }

}
