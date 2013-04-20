<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c\Form;



class Field extends Element
{

	public $name;
	public $value;
	public $exception;



	function __construct($label = null, $name = null, $value = null)
	{
		parent::__construct($label);
        $this->setTemplate(__CLASS__);
		$this->name = $name;
        $this->value = $value;
	}



	function render()
	{
		if (isset($this->form))
		{
			if (!isset($this->value))
			{
				$this->value = $this->form->getValue($this->name);
			}
			if (!isset($this->exception))
			{
				$this->exception = $this->form->getException($this->name);
			}
		}
		parent::render();
	}

}
