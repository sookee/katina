<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c\Form;



class FieldDate extends Field
{

    protected $format;



    function __construct($label = null, $name = null, $format = null)
    {
        parent::__construct($label, $name);
        $this->format = $format;
    }



    function render()
    {
        if (isset($this->format))
        {
            $this->value = date($this->format, strtotime($this->form->getValue($this->name)));
        }
        parent::render();
    }

}
