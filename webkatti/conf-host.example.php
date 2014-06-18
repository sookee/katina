<?php

class Config
{

    # Main title
    static $title = '^6W^7eb^6K^7atti';

    # Minimum deaths for 1v1
    static $minDeathsOvo = 10;

    # Minimum deaths for main page
    static $minDeathsMain = 100;

    # Minimum time for main page (seconds)
    static $minTimeMain = 3600;

    # About HTML
    static $about = '<p>For any questions find us in irc.quakenet.org#openarenahelp or <a href="http://openarenahelp.com/help-chat/">webchat</a>.</p>';

    # Dpmaster cache file (must be writable)
    static $dpmasterCacheFile = '/tmp/dpmaster.xml';

    # Dpmaster cache TTL (seconds)
    static $dpmasterCacheTtl = 86400;

    # URL prefix (without leading and trailing slashes)
    static $urlPrefix = 'webkatti';

    # DB name
    static $dbName = 'oadb';

    # DB host
    static $dbHost = '77.237.250.186';

    # DB user
    static $dbUser = 'oadb';

    # DB password (leave blank if you do not want to change)
    static $dbPass = NULL;

    # Cache
    static $cache = false;

    # Memcache host
    static $memcacheHost = 'localhost';

    # Memcache port
    static $memcachePort = 11211;

    # Memcache TTL (seconds)
    static $memcacheTtl = 600;

    # Admin name
    static $adminName = 'admin';

    # Admin password (leave blank if you do not want to change)
    static $adminPass = '202cb962ac59075b964b07152d234b70';

    # Debug
    static $debug = true;

}
