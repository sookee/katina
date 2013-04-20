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
        ->wrap(new c\Layout());
});

$uri->addPattern('`^$`',                                function (  ) { return wk\c\Main    ::index     (       ); });
$uri->addPattern('`^(clearCache|login|logout|settings|about|changes)$`',
                                                        function ($m) { return wk\c\Main    ::{$m[1]}   (       ); });
$uri->addPattern('`^games$`',                           function (  ) { return wk\c\Game    ::index     (       ); });
$uri->addPattern('`^game-(\d+)$`',                      function ($m) { return wk\c\Game    ::get       ( $m[1] ); });
$uri->addPattern('`^player-(\w+)$`',                    function ($m) { return wk\c\Player  ::get       ( $m[1] ); });

Debug::log()->resources('prepare');



$uri->renderController();

Debug::log()->resources('render');
