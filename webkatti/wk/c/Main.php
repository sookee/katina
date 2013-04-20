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

        $gameIds = M::game()->db()
            ->fields('game_id, game_id')
            ->where('date > now() - interval 1 month')
            ->allK();

        $kills = M::kill()->countByGuid()
            ->key($gameIds, 'game_id')
            ->allK();

        $caps = M::cap()->countByGuid()
            ->key($gameIds, 'game_id')
            ->allK();

        $deaths = M::death()->countByGuid()
            ->key($gameIds, 'game_id')
            ->allK();

        $c->players = new KDCD($kills, $caps, $deaths, M::settings()->get('min_deaths_per_game'));

        return $c;
    }

}
