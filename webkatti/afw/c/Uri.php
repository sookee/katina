<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;

use afw\HttpException;



class BadControllerException extends \Exception {}



class Uri
{

    protected static $instance;
    protected $uri;
    protected $patterns = [];
    protected $exceptionCreator;



    /**
     * @return Uri
     */
    static function instance()
    {
        if (!isset(self::$instance))
        {
            self::$instance = new self();
        }
        return self::$instance;
    }



    static function current()
    {
        return self::instance()->uri;
    }



    /**
     * @throws \afw\HttpException
     * @return Controller
     */
    static function variable($class, $method, $args = [], array $allowable = [])
    {
        if (
            !empty($allowable)
            && !@in_array($method, $allowable[$class])
            && !in_array($class, $allowable)
        ){
            throw new HttpException(404);
        }
        if (!method_exists($class, $method))
        {
            throw new HttpException(404);
        }

        $decoded_args = [];
        foreach ((array)$args as $arg)
        {
            $decoded_args [] = urldecode($arg);
        }

        $controller = call_user_func_array(array($class, $method), $decoded_args);
        if (!($controller instanceof Controller))
        {
            throw new HttpException(404);
        }
        return $controller;
    }



    function __construct($uri = null)
    {
        if (isset($uri))
        {
            $_SERVER['REQUEST_URI'] = $uri;
        }
        $this->uri = $_SERVER['REQUEST_URI'];

        $p = strpos($this->uri, '?');
        if ($p !== false)
        {
            $this->uri = substr($this->uri, 0, $p);
        }

        $this->uri = trim($this->uri, '/');
    }



    function addPattern($pattern, $controllerCreator)
    {
        $this->patterns[$pattern] = $controllerCreator;
    }



    function resetPatterns()
    {
        $this->patterns = [];
    }



    function setException($exceptionControllerCreator)
    {
        $this->exceptionCreator = $exceptionControllerCreator;
    }



    protected function getException(\Exception $e)
    {
        if (empty($this->exceptionCreator))
        {
            throw $e;
        }
        $c = $this->exceptionCreator;
        return $c($e);
    }



    function renderController($try = false)
    {
        $controller = $this->getController($try);
        if (empty($controller))
        {
            return false;
        }
        $controller->render();
        return true;
    }



    /**
     * @throws BadControllerException
     * @return null|\afw\c\Controller
     */
    function getController($try = false)
    {
        $controller = null;
        foreach ($this->patterns as $p => $c)
        {
            if (preg_match($p, $this->uri, $m))
            {
                try
                {
                    $controller = $c($m);
                }
                catch (\Exception $e)
                {
                    $controller = $this->getException($e);
                }
                break;
            }
        }
        if (empty($controller))
        {
            if ($try)
            {
                return null;
            }
            $controller = $this->getException(new HttpException(404));
        }
        if (!($controller instanceof Controller))
        {
            throw new BadControllerException('excepted Controller, got ' . get_class($controller));
        }
        return $controller;
    }

}
