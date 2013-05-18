<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class Event
{

    static $listeners;
    static $targets;

    static function listen($event, $callback, $target = null)
    {
        $key = isset($target) ? spl_object_hash($target) : 0;
        self::$listeners[$key][$event] []= $callback;
        self::$targets[$key] = $target;
    }



    static function unlisten($event, $target = null)
    {
        $key = isset($target) ? spl_object_hash($target) : 0;
        unset(self::$listeners[$key][$event]);
        if (empty(self::$listeners[$key]))
        {
            unset(self::$listeners[$key]);
            unset(self::$targets[$key]);
        }
    }



    static function fire($event, $target = null)
    {
        $key = isset($target) ? spl_object_hash($target) : 0;
        foreach ((array)@self::$listeners[$key][$event] as $callback)
        {
            $callback($target);
        }
    }

}
