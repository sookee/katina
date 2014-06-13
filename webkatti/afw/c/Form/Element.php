<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c\Form;

use afw\c\Controller;



class Element extends Controller
{

    /**
     * @var \afw\c\Form
     */
    public $form;
    public $label;
    public $description;



    function __construct($label = null)
    {
        parent::__construct();
        $this->label = $label;
    }



    function description($description)
    {
        $this->description = $description;
        return $this;
    }



    /**
     * @return Controller
     */
    protected static function simpleElement($__FUNCTION__, Controller $controller)
    {
        $controller->setView(get_called_class() . '::' . $__FUNCTION__);
        return $controller;
    }



    /**
     * @return Element
     */
    static function label($label = null)
    {
        return static::simpleElement(__FUNCTION__, new self($label));
    }



    /**
     * @return Element
     */
    static function header($label = null)
    {
        return static::simpleElement(__FUNCTION__, new self($label));
    }



    /**
     * @return Field
     */
    static function hidden($name = null, $value = null)
    {
        return static::simpleElement(__FUNCTION__, new Field(null, $name, $value));
    }



    /**
     * @return Field
     */
    static function value($label = null, $name = null, $value = null)
    {
        return static::simpleElement(__FUNCTION__, new Field($label, $name, $value));
    }



    /**
     * @return Field
     */
    static function text($label = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new Field($label, $name));
    }



    /**
     * @return Field
     */
    static function color($label = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new Field($label, $name));
    }



    /**
     * @return Field
     */
    static function password($label = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new Field($label, $name));
    }



    /**
     * @return Field
     */
    static function textarea($label = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new Field($label, $name));
    }



    /**
     * @return Field
     */
    static function checkbox($label = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new Field($label, $name));
    }



    /**
     * @return Submit
     */
    static function submit($label = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new Submit($label, $name));
    }



    /**
     * @return FieldDate
     */
    static function date($label = null, $format = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new FieldDate($label, $name, $format));
    }



    /**
     * @return FieldSelect
     */
    static function select($label = null, $options = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new FieldSelect($label, $name, $options));
    }



    /**
     * @return FieldFile
     */
    static function file($label = null, $labelDelete = null, $maxFileSize = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new FieldFile($label, $name, $labelDelete, $maxFileSize));
    }



    /**
     * @return FieldImage
     */
    static function image($label = null, $labelDelete = null, $maxFileSize = null, $name = null)
    {
        return static::simpleElement(__FUNCTION__, new FieldFile($label, $name, $labelDelete, $maxFileSize));
    }

}
