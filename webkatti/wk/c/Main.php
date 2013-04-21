<?php

namespace wk\c;

use M;
use Storage;
use afw\c\Form\Element;



class Main
{

    static function clearCache()
    {
        Storage::cache()->clear();
        header('location: ' . Link::prefix());
        exit;
    }



    static function login()
    {
        $title = 'Authenticate';

        $c = new \afw\c\Form();
        $c->setData($_POST);

        if (!empty($_POST))
        {
            try
            {
                M::supervisor()->authenticate($_POST['name'], $_POST['password']);
                $c->complete();
            }
            catch (ModelFilterException $e)
            {
                $c->setExceptions($e->getExceptions());
            }
            catch (\Exception $e)
            {
                $c->fail($e->getMessage());
            }
        }

        $c->push(Element::header($title));
        $c->push(Element::text('Login', 'name'));
        $c->push(Element::password('Password', 'password'));
        $c->push(Element::submit('Authenticate'));

        return $c->wrap(new Layout($title));
    }



    static function logout()
    {
        \afw\Session::destroy();
        header('location: ' . Link::prefix());
        exit;
    }



    static function about()
    {
        $c = new Layout();
        $c->setTemplate(__METHOD__);
        return $c;
    }



    static function changes()
    {
        $c = new Layout();
        $c->setTemplate(__METHOD__);
        return $c;
    }



    static function settings()
    {
        if (!M::supervisor()->isAuthed())
        {
            throw new \afw\HttpException(404);
        }
        return \afw\c\FormSettings::save(M::settings(), 'Settings saved', 'Settings', true, 'Save')
            ->wrap(new Layout('Settings'));
    }



    static function index()
    {
        $c = new Layout();
        $c->setTemplate(__METHOD__);

        $r = Storage::main()
            ->cache(Storage::cache())
            ->fields('game.game_id, deaths.guid, kills.count as kills, caps.count as caps, deaths.count as deaths, time.count as time')
            ->from('game')
            ->join('deaths',    'game.game_id = deaths.game_id')
            ->join('time',      'game.game_id = time.game_id and deaths.guid = time.guid')
            ->join('kills',     'game.game_id = kills.game_id and deaths.guid = kills.guid')
            ->join('caps',      'game.game_id = caps.game_id and deaths.guid = caps.guid', null, 'left')
            ->where('game.date > now() - interval 1 month and deaths.count > ? and time.count > ?', [
                M::settings()->get('min_deaths_game_main'),
                M::settings()->get('min_time_game_main'),
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
