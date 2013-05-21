<?php

namespace wk\c;

use M;
use Storage;


class Game
{

    static function _list($items)
    {
        return new \afw\c\SimpleList(__METHOD__, $items);
    }



    static function index()
    {
        $c = new Layout();
        $c->setView(__METHOD__);

        $time = strtotime(@$_GET['date']);
        if (empty($time))
        {
            if (!empty($_GET['date']))
            {
                $c->error = 'Wrong value';
            }
            $time = time();
        }
        $c->date = date('Y-m-d', $time);
        
        // List all games of the selected day and count the players using the tables kills & deaths
        $c->games = self::_list(
            Storage::main()->select(
                'select distinct game.*, count(distinct `guid`) AS numPlayers
                from `game` natural join (select * from kills UNION select * from deaths) as kd
                where `date` between ? and ?
                group by `game_id`
                order by game_id desc',
                [
                    $c->date . ' 00:00:00',
                    $c->date . ' 23:59:59'
                ])
        );

        $c->addTitle('Games');
        $c->addTitle(\afw\Utils::datef('d.m.Y', $c->date));

        return $c;
    }



    static function get($id)
    {
        $c = new Layout();
        $c->setView(__METHOD__);

        $c->game = M::game()->get($id);

        $kills = M::kill()->countByGuid()
            ->key($id, 'game_id')
            ->allK();

        $caps = M::cap()->countByGuid()
            ->key($id, 'game_id')
            ->allK();

        $deaths = M::death()->countByGuid()
            ->key($id, 'game_id')
            ->allK();

        $time = M::time()->countByGuid()
            ->key($id, 'game_id')
            ->allK();
        
        $shots = M::weapon_usage()->db()
            ->fields('guid, sum(shots) as shots')
            ->where('game_id=?', $id)
            ->groupBy('guid')
            ->allK();
        
        $hits = M::damage()->db()
            ->fields('guid, sum(hits) as hits')
            ->where('game_id=?', $id)
            ->groupBy('guid')
            ->allK();

        $c->players = new PlayerStats(
            $kills,
            $caps,
            $deaths,
            $time,
            $shots,
            $hits,
            0, // List all players, 
            0  // even those below the death- and time-threshold
        );

        return $c;
    }

}
