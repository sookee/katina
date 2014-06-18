<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class AMemcache extends \Memcache implements ICache
{

    const cacheVersion = 'afw\\m\\AMemcache::version';

    protected $host;
    protected $port;
    protected $timeout;
    protected $prefix;
    protected $prefixInit;
    protected $ttl;
    protected $version;
    protected $versionKey;


    function __construct($host = 'localhost', $port = 11211, $timeout = 1, $prefix = null, $ttl = null)
    {
        $this->host = $host;
        $this->port = $port;
        $this->timeout = $timeout;
        $this->prefixInit = isset($prefix) ? $prefix : $_SERVER['HTTP_HOST'];
        $this->ttl = $ttl;
        $this->version = time();
    }



    function connect()
    {
        if (!isset($this->prefix))
        {
            if (!parent::connect($this->host, $this->port, $this->timeout))
            {
                throw new \Exception('Cannot connect to memcache server');
            }

            $this->versionKey = $this->prefixInit . '.' . self::cacheVersion;
            if (!parent::add($this->versionKey, $this->version))
            {
                $this->version = parent::get($this->versionKey);
            }

            $this->prefix = $this->prefixInit . '.' . $this->version . '.';
        }
    }



    function clear()
    {
        $this->connect();
        $this->version = parent::increment($this->versionKey);
        $this->prefix = $this->prefixInit . '.' . $this->version . '.';
    }



    function add($name, $value, $compress = null, $ttl = null)
    {
        $this->connect();
        return parent::add($this->prefix . $name, $value, $compress, isset($ttl) ? $ttl : $this->ttl);
    }



    function set($name, $value, $compress = null, $ttl = null)
    {
        $this->connect();
        return parent::set($this->prefix . $name, $value, $compress, isset($ttl) ? $ttl : $this->ttl);
    }



    function inc($name, $value = 1)
    {
        $this->connect();
        return parent::increment($this->prefix . $name, $value);
    }



    function get($name)
    {
        $this->connect();
        $r = parent::get($this->prefix . $name);
        return $r === false ? null : $r;
    }



    function dec($name, $value = 1)
    {
        $this->connect();
        return parent::decrement($this->prefix . $name, $value);
    }



    function del($name)
    {
        $this->connect();
        return parent::delete($this->prefix . $name);
    }



    function size()
    {
        $this->connect();
        return parent::getStats()['bytes'];
    }

}
