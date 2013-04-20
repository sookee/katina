<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class Session
{

	protected static $start = 0;
	protected static $read = false;



	static function start()
	{
		if (!self::$start++)
		{
			session_start();
			self::$read = true;
		}
	}



	static function commit()
	{
		if (!--self::$start)
		{
			session_commit();
		}
	}



	static function read()
	{
		if (!self::$read)
		{
            if (isset($_COOKIE[session_name()]))
            {
                session_start();
                session_commit();
            }
			self::$read = true;
		}
	}



	static function get($name)
	{
		self::read();
		return @$_SESSION[$name];
	}



	static function set($name, $value)
	{
		self::start();
		$_SESSION[$name] = $value;
		self::commit();
	}



    static function destroy()
    {
        self::start();
        session_destroy();
        self::commit();
    }


}
