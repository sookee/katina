<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class AAPC implements ICache
{

    protected $prefix;
    protected $ttl;
    protected $clearDelayed;
    protected $clearNeed;


    function __construct($prefix = '', $ttl = 0, $clearDelayed = false)
    {
        $this->prefix = $prefix . '.';
        $this->ttl = $ttl;
        $this->clearDelayed = $clearDelayed;
    }



    function __destruct()
    {
        if ($this->clearNeed)
        {
            $this->_clear();
        }
    }



    protected function _clear()
    {
        foreach (new \APCIterator('user', '`^' . preg_quote($this->prefix) . '`', APC_ITER_KEY) as $key)
        {
            apc_delete($key['key']);
        }
    }



    function clear()
    {
        if ($this->clearDelayed)
        {
            $this->clearNeed = true;
        }
        else
        {
            $this->_clear();
        }
    }



    function add($name, $value)
    {
        return apc_add($this->prefix . $name, $value, $this->ttl);
    }



    function set($name, $value)
    {
        return apc_store($this->prefix . $name, $value, $this->ttl);
    }



    function inc($name, $value = 1)
    {
        return apc_inc($this->prefix . $name, $value);
    }



    function dec($name, $value = 1)
    {
        return apc_dec($this->prefix . $name, $value);
    }



    function get($name)
    {
        $value = apc_fetch($this->prefix . $name, $success);
        return $success ? $value : null;
    }



    function del($name)
    {
        return apc_delete($this->prefix . $name);
    }



    function size()
    {
        return apc_cache_info('user', true)['mem_size'];
    }

}
