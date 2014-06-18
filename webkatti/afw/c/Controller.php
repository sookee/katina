<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;



class Controller
{

    const DIR_CONTROLLER = 'c';
    const DIR_VIEW = 'v';

    private $_templates = [];
    private $_template;
    private $_count;

    /**
     * @var self[]
     */
    private $_wrappers = [];
    private $_wrapper;

    /**
     * @var self[]
     */
    private $_children = [];

    /**
     * @var self
     */
    private $_renderWith;



    function __construct() {}



    function setView($class_or_method)
    {
        $class_or_method = preg_replace(
            '`(^|\\\\)' . static::DIR_CONTROLLER . '\\\\`',
            '$1' . self::DIR_VIEW . '/',
            $class_or_method
        );
        $this->setViewFile(
            str_replace(
                ['\\', '::'],
                ['/', '/'],
                $class_or_method
            ) . '.php'
        );
    }



    protected function setViewFile($path)
    {
        $this->_templates [] = $path;
    }



    function wrap(self $controller)
    {
        $this->_wrappers [] = $controller;
        return $this;
    }



    function push(self $controller)
    {
        $this->_children [] = $controller;
    }



    function contents()
    {
        $this->__render();
    }



    function render()
    {
        $this->_template = 0;
        $this->_count = count($this->_templates);
        $this->_wrapper = count($this->_wrappers);
        $this->__render();
    }



    private function __render()
    {
        if ($this->renderWrappers())
        {
            if ($this->renderTemplates())
            {
                $this->renderChildren();
            }
        }
    }



    private function renderWrappers()
    {
        if ($this->_wrapper == 0)
        {
            return true;
        }
        $this->_wrappers[--$this->_wrapper]->renderWith($this);
        return false;
    }



    private function renderWith(self $controller)
    {
        $this->_renderWith = $controller;
        $this->render();
        unset($this->_renderWith);
    }



    private function renderTemplates()
    {
        if ($this->_template == $this->_count)
        {
            return true;
        }
        include $this->_templates[$this->_template++];
        return false;
    }



    private function renderChildren()
    {
        if (isset($this->_renderWith))
        {
            $this->_renderWith->__render();
        }
        else
        {
            foreach ($this->_children as $child)
            {
                $child->render();
            }
        }
    }

}
