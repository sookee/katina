<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class HttpException extends \Exception
{

	const HEADER_403 = 'HTTP/1.0 403 Forbidden';
	const HEADER_404 = 'HTTP/1.0 404 Not Found';
	const HEADER_500 = 'HTTP/1.0 500 Internal Server Error';



	function __construct($code = 500, $message = null, $previous = null)
	{
		switch ($code)
		{
			case 403:
				header(self::HEADER_403);
				break;
			case 404:
				header(self::HEADER_404);
				break;
			default:
				header(self::HEADER_500);
				break;
		}
		parent::__construct($message, $code, $previous);
	}



}


