<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;



class Settings extends ModelWithFields
{

    const cacheData = 'afw\m\Settings::data';

    protected $file;
    protected $data;



    function __construct($file, \afw\ICache $cache = null)
    {
        $this->file = $file;
        $this->setCache($cache);

        if (isset($this->cache))
        {
            $this->data = $this->cache->get(self::cacheData);
        }

        if (!isset($this->data))
        {
            $this->data = @unserialize(file_get_contents($file));
            $this->cache();
        }
    }



    function save(array $rawValues)
    {
        $newValues = $this->filterValues($rawValues);
        file_put_contents($this->file, serialize($newValues));
        $this->data = $newValues;
        $this->cache();
    }



    private function cache()
    {
        if (isset($this->cache))
        {
            $this->cache->set(self::cacheData, $this->data);
        }
    }



    function get($name)
    {
        return @$this->data[$name];
    }



    function all()
    {
        $settings = [];
        foreach ($this->fields as $name => $field)
        {
            $settings[$name] = @$this->data[$name];
        }
        return $settings;
    }

}
