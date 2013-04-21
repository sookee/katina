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

        $r = \Storage::main()
            ->fields('game.game_id, deaths.guid, kills.count as kills, caps.count as caps, deaths.count as deaths, time.count as time')
            ->from('game')
            ->join('deaths',    'game.game_id = deaths.game_id')
            ->join('time',      'game.game_id = time.game_id and deaths.guid = time.guid')
            ->join('kills',     'game.game_id = kills.game_id and deaths.guid = kills.guid')
            ->join('caps',      'game.game_id = caps.game_id and deaths.guid = caps.guid', null, 'left')
            ->where('game.game_id = ? and deaths.count > ? and time.count > ?', [
                $id,
                M::settings()->get('min_deaths_game'),
                M::settings()->get('min_time_game'),
            ])
            ->groupBy('game_id, guid')
            ->all();

        $players = [];
        foreach ($r as $row)
        {
            $players[$row['guid']] = $row;
        }

        $c->players = new PlayerStats($players);

        return $c;
    }

}
