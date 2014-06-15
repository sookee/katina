<?php

global $microtime;
$microtime = microtime(true);



require 'conf-host.php';
require 'conf-main.php';
require 'conf-models.php';



M::supervisor()->isAuthed();

Debug::log()->resources('start');



$uri = afw\c\Uri::instance();

$uri->setException(function($e)
{
    return (new afw\c\Exception($e))
        ->wrap(new wk\c\Layout());
});

$p = Config::$urlPrefix;

$uri->addPattern("`^({$p}|)$`",             function (  ) { return wk\c\Main    ::index     (       ); });
$uri->addPattern("`^{$p}/(clearCache|login|logout|settings|about|changes)$`",
                                            function ($m) { return wk\c\Main    ::{$m[1]}   (       ); });
$uri->addPattern("`^{$p}/games$`",          function (  ) { return wk\c\Game    ::index     (       ); });
$uri->addPattern("`^{$p}/game-(\d+)$`",     function ($m) { return wk\c\Game    ::get       ( $m[1] ); });
$uri->addPattern("`^{$p}/player-(\w+)$`",   function ($m) { return wk\c\Player  ::get       ( $m[1] ); });

Debug::log()->resources('prepare');



$uri->renderController();

Debug::log()->resources('render');
