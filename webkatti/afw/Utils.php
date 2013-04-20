<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class Utils
{

	static function mailto($to, $from, $subject, $message, $encoding = null, $mime = 'text/html')
	{
		if (empty($encoding))
		{
			$encoding = mb_internal_encoding();
		}
		else
		{
			$message = mb_convert_encoding($message, $encoding);
			$subject = mb_convert_encoding($subject, $encoding);
		}
		$title = $subject;
		$subject = base64_encode($subject);
		$subject = "=?$encoding?B?$subject?=";

		if ($mime == 'text/html')
			$message = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=$encoding\" />
				<title>$title</title></head>
				<body>$message</body></html>";

		if (ini_get('display_errors'))
		{
			file_put_contents('tmp/email'.microtime(true).'.txt', "From: $from\nTo: $to\nSubject: $subject\n\n$message");
		}
		else
		{
			mail($to, $subject, $message, "Content-Type: $mime; charset=$encoding", !empty($from) ? " -f $from" : '');
		}
	}



	static function str2uri($str)
	{
        return trim(
            preg_replace(
                '/[-\s]+/',
                '-',
                preg_replace(
                    '`[^\-\s\da-z]+`',
                    '',
                    transliterator_transliterate(
                        'Any-Latin; NFD; [:Nonspacing Mark:] Remove; NFC; Lower();',
                        $str
                    )
                )
            ),
            '-'
        );
	}



	static function autoreplace($s, $decodeEntities = true, $encoding = null)
	{
		$s = preg_replace([
				'/\<br\s*\/?\>(\S)/i',
				'/(\S)\<p\>/i',
				'/([^\s\<\>\(\-](\<[^\>]*\>)*)\"(?![^\<\>]*\>)/',
				'/\"((\<[^\>]*\>)*[^\s\<\>\)])(?![^\<\>]*\>)/',
				'/(\s)\-\-?\-?(\s+)|&nbsp;\-\-?\-?(\s)/',
				/*'/(\d+)\-(\d+)/',*/
				'/\-\-\-/',
				'/\-\-/',
				'/\<\<|&lt;&lt;/',
				'/\>\>|&gt;&gt;/',
				'/\(c\)/i'
			], [
				"<br />\n$1",
				"$1\n<p>",
				'$1»',
				'«$1',
				'&nbsp;—$2$3',
				/*'$1&minus;$2',*/
				'—',
				'–',
				'«',
				'»',
				'©'
			], $s);
		if ($decodeEntities)
			$s = html_entity_decode($s, null, $encoding ? : mb_internal_encoding());
		return $s;
	}



	static function xhtml($s)
	{
		$s = str_replace(["\r\n", "\r"], "\n", $s);
		$s = str_replace(['<script', '</script'], ['&lt;script', '&lt;/script'], $s);
		$s = preg_replace('/(src\=[\'\"])http\:\/\/'.$_SERVER['HTTP_HOST']
						.'|(href\=[\'\"])http\:\/\/'.$_SERVER['HTTP_HOST'].'/i', '$1$2', $s);
		$s = preg_replace('/(href\=[\'\"]http\:\/\/)/i', 'rel="external" $1', $s);

		$d = new \DOMDocument('1.0', 'UTF-8');
		@$d->loadHTML('<html><head><meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
				<title></title></head><body>'.mb_convert_encoding($s, 'UTF-8').'</body></html>');
		$s = '';
		foreach ($d->documentElement->lastChild->childNodes as $child)
			$s .= $d->saveXML($child);
		return mb_convert_encoding($s, mb_internal_encoding(), 'UTF-8');
	}



	static function cutStr($s, $length)
	{
		$s = strip_tags($s);
		if (mb_strlen($s) > $length)
		{
			$last_is_space = mb_substr($s, $length, 1) == ' ';
			$s = mb_substr($s, 0, $length);
			if (!$last_is_space)
			{
				$s = mb_substr($s, 0, mb_strrpos($s, ' '));
			}
			$s .= mb_substr($s, -1) == '.' ? '..' : '...';
		}
		return $s;
	}



	static function cutHtml($s, $length)
	{
		return ext\HTMLCutter::cut($s, $length);
	}



    static function datef($format, $date)
    {
        return date($format, strtotime($date));
    }



    static function strfdate($format, $date)
    {
        return strftime($format, strtotime($date));
    }

}
