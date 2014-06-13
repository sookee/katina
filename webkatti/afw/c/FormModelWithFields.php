<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;



class FormModelWithFields extends Form
{

    /**
     * @var \afw\m\ModelWithFields
     */
    public $model;



    function __construct($completeMessage, \afw\m\ModelWithFields $model)
    {
        parent::__construct($completeMessage);

        $this->model = $model;
    }



    function pushFields($names = null)
    {
        if (!isset($names) || $names === true)
        {
            foreach ($this->model->fields as $field)
            {
                $controller = $field->getFormField();
                if (isset($controller))
                {
                    $this->push($controller);
                }
            }
        }
        else
        {
            foreach ((array)$names as $i => $v)
            {
                if (is_int($i))
                {
                    $controller = $this->model->getField($v)->getFormField();
                }
                else
                {
                    $controller = Form\Element::value($v, $i);
                }
                if (isset($controller))
                {
                    $this->push($controller);
                }
            }
        }
    }



    function push(Controller $controller)
    {
        if ($controller instanceof Form\FieldFile && !isset($controller->src))
        {
            $controller->src = $this->model->dest($controller->name, 0);
        }
        parent::push($controller);
    }



    function run($callback = null)
    {
        try
        {
            parent::run($callback);
        }
        catch (\afw\m\ModelFilterException $e)
        {
            $this->setExceptions($e->getExceptions());
        }
    }



    function pushElements($headLabel = null, $pushFields = null, $buttonLabel = null)
    {
        if (isset($headLabel))
        {
            $this->push(Form\Element::header($headLabel));
        }
        if (isset($pushFields))
        {
            $this->pushFields($pushFields);
            if (isset($buttonLabel))
            {
                $this->push(Form\Element::submit($buttonLabel));
            }
        }
    }

}
