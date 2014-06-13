<?php

namespace wk\c;

use M;
use Storage;
use afw\c\Form\Element;



class Main
{

    static function clearCache()
    {
        if (\Config::$cache)
        {
            Storage::cache()->clear();
        }
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
        $c->setView(__METHOD__);
        return $c;
    }



    static function changes()
    {
        $c = new Layout();
        $c->setView(__METHOD__);
        return $c;
    }



    static function settings()
    {
        if (!M::supervisor()->isAuthedOrLocal())
        {
            throw new \afw\HttpException(404);
        }
        return \afw\c\FormSettings::save(M::settings(), 'Settings saved', 'Settings', true, 'Save')
            ->wrap(new Layout('Settings'));
    }



    static function index()
    {
        $c = new Layout();
        $c->setView(__METHOD__);

        $db = M::game()->db()
            ->fields('game_id, game_id');

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
        $db->where('date >= ?', $c->dateFrom . ' 00:00:00');

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
        $db->where('date <= ?', $c->dateTo . ' 23:59:59');

        $gameIds = $db->allK();

        if (!empty($gameIds))
        {
            $kills = M::kill()->countByGuid()
                ->key($gameIds, 'game_id')
                ->allK();

            $caps = M::cap()->countByGuid()
                ->key($gameIds, 'game_id')
                ->allK();

            $deaths = M::death()->countByGuid()
                ->key($gameIds, 'game_id')
                ->allK();

            $time = M::time()->countByGuid()
                ->key($gameIds, 'game_id')
                ->allK();

            $shots = M::weapon_usage()->db()
                ->fields('guid, sum(shots) as shots')
                ->groupBy('guid')
                ->key($gameIds, 'game_id')
                ->allK();

            $hits = M::damage()->db()
                ->fields('guid, sum(hits) as hits')
                ->groupBy('guid')
                ->key($gameIds, 'game_id')
                ->allK();
        }
        else
        {
            $kills = [];
            $caps = [];
            $deaths = [];
            $time = [];
            $shots = [];
            $hits = [];
        }

        $c->players = new PlayerStats(
            $kills,
            $caps,
            $deaths,
            $time,
            $shots,
            $hits,
            \Config::$minDeathsMain,
            \Config::$minTimeMain
        );

        return $c;
    }

}
