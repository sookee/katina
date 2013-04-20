<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class FileParams
{

	protected $params = [];
	protected $index = 0;



	function export()
	{
		return $this->params;
	}



	protected function setParam($param, $value)
	{
		$this->params[$this->index][$param] = $value;
		return $this;
	}



	function dest($value)
	{
		$this->index = @count($this->params);
		return $this->setParam(__FUNCTION__, $value);
	}



	function imagick($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function width($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function height($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function background($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function transparent($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function quality($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function reduction($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function watermark($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function wm_width($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function wm_height($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function wm_left($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function wm_top($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}



	function format($value)
	{
		return $this->setParam(__FUNCTION__, $value);
	}

}


