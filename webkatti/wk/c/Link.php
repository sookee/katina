<?php

namespace wk\c;

use \Config;



class Link
{

    static function prefix($link = '')  {return '/' . Config::$urlPrefix . $link;}
    static function game($id)           {return self::prefix('/game-' . $id);}
    static function player($guid)       {return self::prefix('/player-' . $guid);}

}
