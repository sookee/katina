<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;



class SimpleList extends Controller
{

    public $items;



    function __construct($__METHOD__, &$items)
    {
        parent::__construct();
        $this->setView($__METHOD__);
        $this->items = & $items;
    }



    function getItems()
    {
        return empty($this->items) ? [] : $this->items;
    }



    function isEmpty()
    {
        return empty($this->items);
    }

}
