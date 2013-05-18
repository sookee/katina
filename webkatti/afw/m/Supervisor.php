<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;

use afw\Session;



class Supervisor
{

    const SESSION_AUTHED = 'afw\m\Supervisor::authed';

    protected $passwords = [];
    protected $options = [];
    protected $authed;
    protected $algo;



    function __construct($algo = null)
    {
        $this->algo = $algo;
        $this->authed = Session::get(self::SESSION_AUTHED);
    }



    function add($name, $password, array $options = [])
    {
        $this->passwords[$name] = $password;
        $this->options[$name] = $options;
        return $this;
    }



    function authenticate($name, $password)
    {
        $this->authed = @$this->passwords[$name] == $this->hash($password) ? $name : null;
        Session::set(self::SESSION_AUTHED, $this->authed);
        return isset($this->authed);
    }



    private function hash($password)
    {
        if (isset($this->algo))
        {
            $password = hash($this->algo, $password);
        }
        return $password;
    }



    function isAuthed()
    {
        return isset($this->authed);
    }



    function getName()
    {
        return $this->authed;
    }



    function getOption($name)
    {
        return @$this->options[$this->authed][$name];
    }

}
