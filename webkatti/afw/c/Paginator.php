<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;



class Paginator extends Controller
{

	public $count;
	public $current;
	public $radius = 3;
    public $reverse = false;

	protected $prefix;
	protected $urlStart;
	protected $urlEnd;



	function __construct($count = 0, $current = 1, $prefix = 'page-')
	{
		parent::__construct();
		$this->setTemplate(__CLASS__);
		$this->count = $count;
		$this->current = empty($current) ? 1 : $current;
		$this->prefix = $prefix;
		$this->urlStart = preg_replace('`/' . preg_quote($this->prefix) . '\d+$`', '',
            parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH));
		$this->urlEnd = empty($_SERVER['QUERY_STRING']) ? '' : '?' . $_SERVER['QUERY_STRING'];
	}



	function href($page)
	{
		$href = $this->urlStart;
		if (
            (!$this->reverse && $page != 1)
            || ($this->reverse && $page != $this->count)
		){
			$href .= '/'.$this->prefix.$page;
		}
		return $href.$this->urlEnd;
	}

}
