<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;

use afw\HttpException;



class Exception extends Controller
{

	/**
	 * @var \Exception
	 */
	public $exception;
	public $title;
	public $title403 = '403 Forbidden';
	public $title404 = '404 Not Found';
	public $title500 = '500 Internal Server Error';




	function __construct(\Exception $exception)
	{
		parent::__construct();

		$this->setTemplate(__CLASS__);
		$this->exception = $exception;

		if ($exception instanceof HttpException)
		{
			switch ($exception->getCode())
			{
				case 403:
					$this->title = $this->title403;
					break;
				case 404:
					$this->title = $this->title404;
					break;
			}
		}
		if (empty($this->title))
		{
			header(HttpException::HEADER_500);
			$this->title = $this->title500;
		}
	}



	function wrap(Controller $controller)
	{
		if ($controller instanceof Layout)
		{
			$controller->addTitle($this->title);
		}
		return parent::wrap($controller);
	}

}
