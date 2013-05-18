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

    private $templates = [];
    private $template;
    private $count;

    /**
     * @var self[]
     */
    private $wrappers = [];
    private $wrapper;

    /**
     * @var self[]
     */
    private $children = [];

    /**
     * @var self
     */
    private $renderWith;



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
        $this->templates [] = $path;
    }



    function wrap(self $controller)
    {
        $this->wrappers [] = $controller;
        return $this;
    }



    function push(self $controller)
    {
        $this->children [] = $controller;
    }



    function contents()
    {
        $this->__render();
    }



    function render()
    {
        $this->template = 0;
        $this->count = count($this->templates);
        $this->wrapper = count($this->wrappers);
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
        if ($this->wrapper == 0)
        {
            return true;
        }
        $this->wrappers[--$this->wrapper]->renderWith($this);
        return false;
    }



    private function renderWith(self $controller)
    {
        $this->renderWith = $controller;
        $this->render();
        unset($this->renderWith);
    }



    private function renderTemplates()
    {
        if ($this->template == $this->count)
        {
            return true;
        }
        include $this->templates[$this->template++];
        return false;
    }



    private function renderChildren()
    {
        if (isset($this->renderWith))
        {
            $this->renderWith->__render();
        }
        else
        {
            foreach ($this->children as $child)
            {
                $child->render();
            }
        }
    }

}
