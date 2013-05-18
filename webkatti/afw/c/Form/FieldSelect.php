<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c\Form;



class FieldSelect extends Field
{

    public $options;
    public $padding;



    function __construct($label = null, $name = null, $options = null, $padding = ' ·  ')
    {
        parent::__construct($label, $name);
        $this->options = $options;
        $this->padding = $padding;
    }



    function render()
    {
        if (isset($this->options) && is_callable($this->options))
        {
            $options = $this->options;
            $this->options = $options();
        }
        $this->options = (array)$this->options;
        parent::render();
    }



    function renderOption($i, $v, $deep = 0)
    {
        if (is_array($v))
        {
            echo '<option disabled="disabled">',
                str_repeat($this->padding, $deep),
                $i, '</option>';
            foreach ($v as $ci => $cv)
            {
                $this->renderOption($ci, $cv, $deep + 1);
            }
        }
        else
        {
            echo '<option value="', htmlspecialchars($i), '"',
                (string)$i == (string)$this->value ? ' selected="selected"' : ''
                , '>',
                str_repeat($this->padding, $deep),
                $v, '</option>';
        }
    }

}
