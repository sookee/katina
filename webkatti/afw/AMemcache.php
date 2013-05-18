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

    protected $conn;
    protected $prefix;
    protected $prefixInit;
    protected $ttl;
    protected $version = 1;



    function __construct($prefix = '', $ttl = 0)
    {
        $this->prefixInit = $prefix;
        $this->ttl = $ttl;

        if (!parent::add(self::cacheVersion, $this->version))
        {
            $this->version = parent::get(self::cacheVersion);
        }

        $this->prefix = $this->prefixInit . '.' . $this->version . '.';
    }



    function clear()
    {
        $this->version = parent::inc(self::cacheVersion);
        $this->prefix = $this->prefixInit . '.' . $this->version . '.';
    }



    function add($name, $value, $compress = null, $ttl = null)
    {
        return parent::add($this->prefix . $name, $value, $compress, isset($ttl) ? $ttl : $this->ttl);
    }



    function set($name, $value, $compress = null, $ttl = null)
    {
        return parent::set($this->prefix . $name, $value, $compress, isset($ttl) ? $ttl : $this->ttl);
    }



    function inc($name, $value = 1)
    {
        return parent::increment($this->prefix . $name, $value);
    }



    function get($name)
    {
        $r = parent::get($this->prefix . $name);
        return $r === false ? null : $r;
    }



    function dec($name, $value = 1)
    {
        return parent::decrement($this->prefix . $name, $value);
    }



    function del($name)
    {
        return parent::delete($this->prefix . $name);
    }



    function size()
    {
        return parent::getStats()['bytes'];
    }

}
