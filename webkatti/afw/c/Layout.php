<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;



class Layout extends Controller
{

    const FORMAT_CSS = '<link rel="stylesheet" href="%s" />';
    const FORMAT_JS = '<script src="%s"></script>';
    const FORMAT_JS_CODE = '<script>%s</script>';

    public $title;
    public $titleSeparator = ' | ';
    public $keywords;
    public $description;
    public $head;
    public $body;
    public $resourcesBase = '/';
    public $resourcesDir = '';
    
    protected $css = [];
    protected $js = [];



    function __construct()
    {
        parent::__construct();
        $this->setView(__CLASS__);
    }



    function addTitle($title)
    {
        foreach ((array)$title as $v)
        {
            $this->title = $v . (isset($this->title) ? $this->titleSeparator . $this->title : '');
        }
    }



    function addCss($css)
    {
        $this->css[$css] = true;
    }



    function addJs($js)
    {
        $this->js[$js] = null;
    }



    function addJsCode($code)
    {
        $this->js [] = $code;
    }



    function css()
    {
        foreach ($this->css as $css => $true)
        {
            echo "\t";
            printf(self::FORMAT_CSS, $this->resourceUrl($css));
            echo "\n";
        }
    }



    function js()
    {
        foreach ($this->js as $js => $code)
        {
            echo "\t";
            if (isset($code))
            {
                printf(self::FORMAT_JS_CODE, $code);
            }
            else
            {
                printf(self::FORMAT_JS, $this->resourceUrl($js));
            }
            echo "\n";
        }
    }



    protected function resourceUrl($fname)
    {

        if (parse_url($fname, PHP_URL_HOST))
        {
            return $fname;
        }
        else
        {
            return $this->resourcesBase . $fname . '?' . filemtime($this->resourcesDir . $fname);
        }
    }



    function setKeywordsFromStr($str)
    {
        $str = preg_replace('`\W+`u', ' ', $str);
        $words = [];
        foreach (preg_split('`\s+`', $str) as $word)
        {
            if (mb_strlen($word) > 2)
            {
                $words [] = $word;
            }
        }
        $this->keywords = implode(', ', $words);
    }



    function setKeywordsFromTitle()
    {
        $this->setKeywordsFromStr($this->title);
    }

}
