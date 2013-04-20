<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c\Form;



class FieldFile extends Field
{

	public $labelDelete;
	public $maxFileSize;
	public $src;



	function __construct($label = null, $name = null, $labelDelete = null, $maxFileSize = null)
	{
		parent::__construct($label, $name);
		$this->labelDelete = $labelDelete;
		$this->maxFileSize = $maxFileSize;
	}

}
