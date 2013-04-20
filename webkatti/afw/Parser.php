<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class Parser
{

    public $timeout;
    public $sleep;
    public $encoding;
    public $url;
    public $contents;
    public $tail;



    protected function preload()
    {
        if (isset($this->timeout))
        {
            set_time_limit($this->timeout);
        }
        if (isset($this->sleep))
        {
            sleep($this->sleep);
        }
    }


    function load($url)
    {
        $this->preload();
        $this->url = $url;
        $this->tail = $this->contents = mb_convert_encoding(
            file_get_contents($url),
            mb_internal_encoding(),
            $this->encoding
        );
    }



    function reset()
    {
        $this->tail = $this->contents;
    }



    function parseTo($str, $try = false)
    {
        if (($pos = stripos($this->tail, $str)) === false)
        {
            return false;
        }
        $result = substr($this->tail, 0, $pos);
        if (!$try)
        {
            $this->tail = substr($this->tail, $pos + strlen($str));
        }
        return $result;
    }



    function rparseTo($str, $try = false)
    {
        if (($pos = strripos($this->tail, $str)) === false)
        {
            return false;
        }
        $result = substr($this->tail, $pos + strlen($str));
        if (!$try)
        {
            $this->tail = substr($this->tail, 0, $pos);
        }
        return $result;
    }

}
