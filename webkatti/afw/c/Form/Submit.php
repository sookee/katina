<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c\Form;



class Submit extends Element
{

    protected static $i = 0;
    public $name;



    function __construct($label = null, $name = null)
    {
        parent::__construct($label);

        if (!isset($name))
        {
            $name = '__submit' . ++self::$i;
        }
        $this->name = $name;
    }

}
