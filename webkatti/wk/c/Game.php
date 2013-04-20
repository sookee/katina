<?php

namespace wk\c;

use M;



class Game
{

    static function _list($items)
    {
        return new \afw\c\SimpleList(__METHOD__, $items);
    }



    static function index()
    {
        $c = new Layout();
        $c->setTemplate(__METHOD__);

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

        $c->games = self::_list(
            M::game()->db()
                ->where('`date` between ? and ?', [
                    $c->date . ' 00:00:00',
                    $c->date . ' 23:59:59'
                ])
                ->orderBy('game_id desc')
                ->all()
        );

        $c->addTitle('Games');
        $c->addTitle(\afw\Utils::datef('d.m.Y', $c->date));

        return $c;
    }



    static function get($id)
    {
        $c = new Layout();
        $c->setTemplate(__METHOD__);

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

        $c->players = new KDCD($kills, $caps, $deaths);

        return $c;
    }

}
