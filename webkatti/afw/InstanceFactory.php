<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class InstanceFactory
{

    private static $instances = [];



    protected static function instance($__FUNCTION__, $instance)
    {
        if (!isset(self::$instances[$__FUNCTION__]))
        {
            if (array_key_exists($__FUNCTION__, self::$instances))
            {
                throw new \Exception("instance '$__FUNCTION__' is null or not yet created");
            }
            $instance(self::$instances[$__FUNCTION__]);
        }
        return self::$instances[$__FUNCTION__];
    }

}
