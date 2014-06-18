<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
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
        {
            $message = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=$encoding\" />
                <title>$title</title></head>
                <body>$message</body></html>";
        }

        if (ini_get('display_errors'))
        {
            file_put_contents(
                'tmp/email' . microtime(true) . '.txt',
                "From: $from\nTo: $to\nSubject: $subject\n\n$message"
            );
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
            /* '/(\d+)\-(\d+)/', */
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
            ' —$2$3',
            /* '$1&minus;$2', */
            '—',
            '–',
            '«',
            '»',
            '©'
        ], $s);
        if ($decodeEntities) $s = html_entity_decode($s, null, $encoding ? : mb_internal_encoding());
        return $s;
    }



    static function xhtml($s)
    {
        $s = str_replace(["\r\n", "\r"], "\n", $s);
        $s = str_replace(['<script', '</script'], ['&lt;script', '&lt;/script'], $s);
        $s = preg_replace('/(src\=[\'\"])http\:\/\/' . $_SERVER['HTTP_HOST']
            . '|(href\=[\'\"])http\:\/\/' . $_SERVER['HTTP_HOST'] . '/i', '$1$2', $s);
        $s = preg_replace('/(href\=[\'\"]http\:\/\/)/i', 'rel="external" $1', $s);

        $d = new \DOMDocument('1.0', 'UTF-8');
        @$d->loadHTML('<html><head><meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
            <title></title></head><body>' . mb_convert_encoding($s, 'UTF-8') . '</body></html>');
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



    static function bestLocale(array $available_locales, $http_accept_language = null)
    {
        if (!isset($http_accept_language))
        {
            $http_accept_language = @$_SERVER['HTTP_ACCEPT_LANGUAGE'];
        }

        $available_prefixes = array();
        foreach ($available_locales as $locale)
        {
            $parts = explode('_', $locale);
            $available_prefixes[$parts[0]] = $locale;
        }

        preg_match_all(
            '/([[:alpha:]]{1,8})(-([[:alpha:]|-]{1,8}))?(\s*;\s*q\s*=\s*(1\.0{0,3}|0\.\d{0,3}))?\s*(,|$)/i',
            $http_accept_language,
            $hits,
            PREG_SET_ORDER
        );

        $bestlocale = $available_locales[0];
        $bestqval = 0;

        foreach ($hits as $arr)
        {
            $locale = $prefix = strtolower($arr[1]);
            if (!empty($arr[3]))
            {
                $locale .= '_' . strtoupper($arr[3]);
            }

            $qvalue = empty($arr[5]) ? 1.0 : floatval($arr[5]);

            if (in_array($locale, $available_locales) && ($qvalue > $bestqval))
            {
                $bestlocale = $locale;
                $bestqval = $qvalue;
            }
            else if (isset($available_prefixes[$prefix]) && (($qvalue * 0.9) > $bestqval))
            {
                $bestlocale = $available_prefixes[$prefix];
                $bestqval = $qvalue * 0.9;
            }
        }

        return $bestlocale;
    }



    static function setLocale($locale, array $available, $encoding = 'UTF-8')
    {
        if (!in_array($locale, $available))
        {
            $locale = \afw\Utils::bestLocale($available);
        }
        $locale .= '.' . $encoding;
        setlocale(LC_ALL, $locale);
        putenv('LC_ALL=' . $locale);
        return $locale;
    }

}
