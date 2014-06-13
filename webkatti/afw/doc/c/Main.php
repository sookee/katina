<?php

namespace afw\doc\c;



class Main extends \afw\c\Controller
{

    public $text;



    function __construct(\afw\c\Layout $layout, $text = null, $dark = false)
    {
        parent::__construct();
        $this->setView(__CLASS__);

        $this->wrap($layout);

        if ($dark)
        {
            $layout->addCss('afw/res/css/default-dark.css');
            $layout->addCss('afw/res/css/google-code-prettify-dark.css');
        }
        else
        {
            $layout->addCss('afw/res/css/default-light.css');
            $layout->addCss('afw/res/css/google-code-prettify-light.css');
        }

        $layout->addJs('afw/res/js/APrettyPrint.js');
        $layout->addJsCode('window.onload = function() {APrettyPrint();};');

        $this->text = $text;
    }



    static function doc(\afw\c\Layout $layout, $file, $dark = false)
    {
        $markup = new \afw\AMarkup(50, 1);
        $c = new self($layout, $markup->convert(file_get_contents($file)), $dark);
        $c->setView(__METHOD__);
        return $c;
    }



    static function exampleCSS(\afw\c\Layout $layout, $dark = false)
    {
        $c = new self($layout, null, $dark);
        $c->setView(__METHOD__);
        return $c;
    }


    
    static function exampleABigImage(\afw\c\Layout $layout, $dark = false)
    {
        $c = new self($layout, null, $dark);
        $c->setView(__METHOD__);

        $layout->addCss('afw/res/css/ABigImage.css');

        return $c;
    }

}
