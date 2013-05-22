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


        $c->dateFrom = strtotime(@$_GET['from']);
        if (empty($c->dateFrom))
        {
            if (!empty($_GET['from']))
            {
                $c->error = 'Wrong value';
            }
            $c->dateFrom = time() - 60 * 60 * 24 * 30;
        }
        $c->dateFrom = date('Y-m-d', $c->dateFrom);


        $c->dateTo = strtotime(@$_GET['to']);
        if (empty($c->dateTo))
        {
            if (!empty($_GET['to']))
            {
                $c->error = 'Wrong value';
            }
            $c->dateTo = time();
        }
        $c->dateTo = date('Y-m-d', $c->dateTo);


        // List all games of the selected day and count the players using the tables kills & deaths
        $c->games = self::_list(
            Storage::main()->select(
                'select distinct game.*, count(distinct `guid`) AS numPlayers
                from `game` natural join (select * from kills UNION select * from deaths) as kd
                where `date` between ? and ?
                group by `game_id`
                order by game_id desc',
                [
                    $c->dateFrom,
                    $c->dateTo
                ])
        );

        $c->addTitle('Games');

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
