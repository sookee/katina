<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;

use afw\Session;



class Form extends Controller
{

	const SESSION_COMPLETE = 'afw\c\Form::complete';
	const SESSION_REFERER = 'afw\c\Form::referer';

	protected $data = [];
	protected $default = [];
	protected $exceptions = [];
	protected $elements = [];

	public $action;
	public $method;
	public $maxFileSize;
	public $error;
	public $complete = false;
	public $completeMessage;




	function __construct($completeMessage = '', $action = '', $method = 'post')
	{
		parent::__construct();

		$this->setTemplate(__CLASS__);
		$this->completeMessage = $completeMessage;
		$this->action = $action;
		$this->method = $method;

		if (empty($_POST) && !empty($_SERVER['HTTP_REFERER']))
		{
			Session::set(self::SESSION_REFERER, $_SERVER['HTTP_REFERER']);
		}
	}



    function returnToUrl($url = null)
    {
        Session::set(self::SESSION_REFERER, isset($url) ? $url : $_SERVER['REQUEST_URI']);
    }



	function setData($data)
	{
		$this->data = (array)$data;
	}



	function setDefault($default)
	{
		$this->default = (array)$default;
	}



	function setExceptions($exceptions)
	{
		$this->exceptions = array_merge($this->exceptions, (array)$exceptions);
	}



	function addException($name, \Exception $exception)
	{
		$this->exceptions[$name] = $exception;
	}



	function getValue($name)
	{
		return isset($this->data[$name]) ? $this->data[$name] : @$this->default[$name];
	}



    function setValue($name, $value)
    {
        $this->data[$name] = $value;
    }



    function setDefaultValue($name, $value)
    {
        $this->default[$name] = $value;
    }



	function getException($name)
	{
		return @$this->exceptions[$name]
			? $this->exceptions[$name]->getMessage()
			: '';
	}



	static function lastComplete($noClear = false)
	{
        $message = Session::get(self::SESSION_COMPLETE);
        if (!$noClear && !empty($message))
        {
            Session::set(self::SESSION_COMPLETE, null);
        }
		return $message;
	}



	function complete($message = null)
	{
		if (empty($message))
		{
			$message = $this->completeMessage;
		}
		Session::set(self::SESSION_COMPLETE, $message);
		if (!empty($_SESSION[self::SESSION_REFERER]))
		{
			header('location: '.$_SESSION[self::SESSION_REFERER]);
			exit;
		}
		$this->complete = $message;
	}



	function fail($message)
	{
		$this->error = $message;
	}



	function push(Controller $controller)
	{
		if ($controller instanceof Form\Element)
		{
			$controller->form = $this;

			if (
				$controller instanceof Form\FieldFile
				&& $controller->maxFileSize > $this->maxFileSize
			){
				$this->maxFileSize = $controller->maxFileSize;
			}
		}
		parent::push($controller);
	}

}
