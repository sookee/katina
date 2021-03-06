<?php

ini_set('display_errors', Config::debug);
set_include_path('.');
setlocale(LC_ALL, 'en_US.UTF-8');
putenv('LC_ALL=en_US.UTF-8');
mb_internal_encoding('UTF-8');
date_default_timezone_set('GMT');



function __autoload($class)
{
    require str_replace('\\', '/', $class) . '.php';
}



class Debug extends afw\InstanceFactory
{

    /**
     * @return afw\DebugLog
     */
    static function log()
    {
        return self::instance(__FUNCTION__, function(&$log)
        {
            global $microtime;
            $log = new afw\DebugLog($microtime);
            $log->enabled = Config::debug;
            $log->jsConsole = true;
//            $log->echo = true;
        });
    }

}



class Storage extends afw\InstanceFactory
{

    /**
     * @return afw\AAPC
     */
    static function cache()
    {
        return !Config::cache ? null : self::instance(__FUNCTION__, function(&$cache)
        {
            $cache = new afw\AAPC('', Config::cacheTtl);
        });
    }



    /**
     * @return afw\APDO
     */
    static function main()
    {
        return self::instance(__FUNCTION__, function(&$db)
        {
            $db = new afw\APDO('mysql:host=' . Config::dbHost . ';dbname=' . Config::dbName,
                Config::dbUser, Config::dbPass,
                [
                    \PDO::MYSQL_ATTR_INIT_COMMAND => 'SET NAMES "utf8"',
                    \PDO::ATTR_ERRMODE => \PDO::ERRMODE_EXCEPTION
                ]
            );

            if (Config::debug)
            {
                $db->setLog(Debug::log());
            }
        });
    }

}
