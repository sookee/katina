<?php

namespace wk\c;

use M;
use Storage;



class Player
{

    static function get($guid)
    {
        $c = new Layout();
        $c->setTemplate(__METHOD__);

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

            $r = M::kill()->db()
                ->fields('weap, sum(count) as count')
                ->where("game_id in ($gameIds) and guid=?", $guid)
                ->groupBy('weap')
                ->orderBy('count desc')
                ->allK();

            $sum = array_sum($r);
            foreach ($r as $weap => $count)
            {
                $c->weapons[$weap]['kills'] = round($count / $sum * 100) ? : '';
            }

            $r = M::death()->db()
                ->fields('weap, sum(count) as count')
                ->where("game_id in ($gameIds) and guid=?", $guid)
                ->groupBy('weap')
                ->allK();

            $sum = array_sum($r);
            foreach ($r as $weap => $count)
            {
                $c->weapons[$weap]['deaths'] = round($count / $sum * 100) ? : '';
            }
        }

        $c->addTitle('Player');
        $c->addTitle(\wk\Utils::uncolor($c->name));

        return $c;
    }

}
