<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



abstract class ParserList extends Parser
{

    protected $parseListStart;
    protected $parseItemStart;
    protected $item;



    abstract protected function parseItem();



    function parseList($url)
    {
        $r = [];
        $this->load($url);
        $this->parseTo($this->parseListStart);
        while ($this->parseTo($this->parseItemStart) !== false)
        {
            $this->item = null;
            $this->parseItem();
            if (isset($this->item))
            {
                $r []= $this->item;
            }
        }
        return $r;
    }

}
