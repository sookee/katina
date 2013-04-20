<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;



class FormSettings extends FormModelWithFields
{

	/**
	 * @return FormSettings
	 */
	static function save(\afw\m\Settings $settings,
        $completeMessage, $headLabel = null, $pushFields = null, $buttonLabel = null)
	{
		$c = new static($completeMessage, $settings);
		$c->setData($_POST);
		$c->setDefault($settings->all());
        $c->run(function() use ($settings)
        {
            $settings->save($_POST);
        });
        $c->pushElements($headLabel, $pushFields, $buttonLabel);
		return $c;
	}

}
