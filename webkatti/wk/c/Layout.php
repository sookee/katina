<?php

namespace wk\c;



class Layout extends \afw\c\Layout
{

    function __construct($title = null)
    {
        parent::__construct();
        $this->setTemplate(__CLASS__);

        $this->addTitle('WebKatti OpenArena statistics');

        if (isset($title))
        {
            $this->addTitle($title);
        }

        $this->addCss('afw/res/css/default-dark.css');
        $this->addCss('wk/res/main.css');
//        $this->addJs('wk/res/main.js');
        $this->addJs('http://code.jquery.com/jquery-1.9.1.min.js');
        $this->addJs('wk/res/jquery.tablesorter.min.js');
        $this->addJsCode('$(document).ready(function(){$("table").tablesorter();});');

        $this->head = '<link rel="shortcut icon" href="/favicon.ico">';
    }

}
