<?php

use afw\c\Form\Element;



class M extends \afw\InstanceFactory
{

    /**
     * @return afw\m\Settings
     */
    static function settings()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new afw\m\Settings('conf-host.php');

            $m->addField('title')
                ->required()
                ->setFormField(Element::text('Main title'));

            $m->addField('minDeathsOvo')
                ->required()
                ->nullint()
                ->setFormField(Element::text('Minimum deaths for 1v1'));

            $m->addField('minDeathsMain')
                ->required()
                ->nullint()
                ->setFormField(Element::text('Minimum deaths for main page'));

            $m->addField('minTimeMain')
                ->required()
                ->nullint()
                ->setFormField(Element::text('Minimum time for main page (seconds)'));

            $m->addField('about')
                ->setFormField(Element::textarea('About HTML'));

            $m->addField('dpmasterCacheFile')
                ->required()
                ->setFormField(Element::text('Dpmaster cache file (must be writable)'));

            $m->addField('dpmasterCacheTtl')
                ->nullint()
                ->setFormField(Element::text('Dpmaster cache TTL (seconds)'));

            $m->addField('urlPrefix')
                ->setFormField(Element::text('URL prefix (without leading and trailing slashes)'));

            $m->addField('dbName')
                ->required()
                ->setFormField(Element::text('DB name'));

            $m->addField('dbHost')
                ->required()
                ->setFormField(Element::text('DB host'));

            $m->addField('dbUser')
                ->required()
                ->setFormField(Element::text('DB user'));

            $m->addField('dbPass')
                ->emptyskip()
                ->setFormField(Element::password('DB password (leave blank if you do not want to change)'));

            $m->addField('cache')
                ->bool()
                ->setFormField(Element::select('Cache', function() {return [false => 'No', true => 'Yes'];}));

            $m->addField('memcacheHost')
                ->setFormField(Element::text('Memcache host'));

            $m->addField('memcachePort')
                ->nullint()
                ->setFormField(Element::text('Memcache port'));

            $m->addField('memcacheTtl')
                ->nullint()
                ->setFormField(Element::text('Memcache TTL (seconds)'));

            $m->addField('adminName')
                ->setFormField(Element::text('Admin name'));

            $m->addField('adminPass')
                ->emptyskip()
                ->hash('md5')
                ->setFormField(Element::password('Admin password (leave blank if you do not want to change)'));

            $m->addField('debug')
                ->bool()
                ->setFormField(Element::select('Debug', function() {return [false => 'No', true => 'Yes'];}));
        });
    }



    /**
     * @return afw\m\Supervisor
     */
    static function supervisor()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new afw\m\Supervisor('md5');

            $m->add(\Config::$adminName, \Config::$adminPass);
        });
    }


    /**
     * @return afw\m\Model
     */
    static function game()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new afw\m\Model(Storage::main(), 'game');
            $m->pkey = 'game_id';
            $m->setCache(Storage::cache());

            $m->addField('host');
            $m->addField('port');
            $m->addField('date');
            $m->addField('map');
        });
    }


    /**
     * @return afw\m\Model
     */
    static function player()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new afw\m\Model(Storage::main(), 'player');
            $m->pkey = ['guid', 'name'];
            $m->setCache(Storage::cache());

            $m->addField('count');
        });
    }


    /**
     * @return wk\m\User
     */
    static function user()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new wk\m\User(Storage::main(), 'user', M::player());
            $m->pkey = 'guid';
            $m->setCache(Storage::cache());

            $m->addField('name');
        });
    }


    /**
     * @return wk\m\Action
     */
    static function kill()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new wk\m\Action(Storage::main(), 'kills');
            $m->pkey = ['game_id', 'guid'];
            $m->setCache(Storage::cache());

            $m->addField('weap');
            $m->addField('count');

            $m->addReference(M::game(), 'kills', 'game', 'game_id');
        });
    }


    /**
     * @return wk\m\Action
     */
    static function death()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new wk\m\Action(Storage::main(), 'deaths');
            $m->pkey = ['game_id', 'guid'];
            $m->setCache(Storage::cache());

            $m->addField('weap');
            $m->addField('count');

            $m->addReference(M::game(), 'deaths', 'game', 'game_id');
        });
    }


    /**
     * @return wk\m\Action
     */
    static function cap()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new wk\m\Action(Storage::main(), 'caps');
            $m->pkey = ['game_id', 'guid'];
            $m->setCache(Storage::cache());

            $m->addField('count');

            $m->addReference(M::game(), 'caps', 'game', 'game_id');
        });
    }


    /**
     * @return wk\m\Action
     */
    static function ovo()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new wk\m\Action(Storage::main(), 'ovo');
            $m->pkey = ['game_id', 'guid1', 'guid2'];
            $m->setCache(Storage::cache());

            $m->addField('count');

            $m->addReference(M::game(), 'ovos', 'game', 'game_id');
        });
    }


    /**
     * @return wk\m\Action
     */
    static function time()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new wk\m\Action(Storage::main(), 'time');
            $m->pkey = ['game_id', 'guid'];
            $m->setCache(Storage::cache());

            $m->addField('count');

            $m->addReference(M::game(), 'times', 'game', 'game_id');
        });
    }


    /**
     * @return wk\m\Action
     */
    static function damage()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new wk\m\Action(Storage::main(), 'damage');
            $m->pkey = ['game_id', 'guid', 'mod'];
            $m->setCache(Storage::cache());

            $m->addField('hits');
            $m->addField('dmgDone');
            $m->addField('hitsRecv');
            $m->addField('dmgRecv');
        });
    }


    /**
     * @return wk\m\Action
     */
    static function playerstats()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new wk\m\Action(Storage::main(), 'playerstats');
            $m->pkey = ['game_id', 'guid'];
            $m->setCache(Storage::cache());

            $m->addField('fragsFace');
            $m->addField('fragsBack');
            $m->addField('fraggedInFace');
            $m->addField('fraggedInBack');
            $m->addField('spawnKillsDone');
            $m->addField('spawnKillsRecv');
            $m->addField('pushesDone');
            $m->addField('pushesRecv');
            $m->addField('healthPickedUp');
            $m->addField('armorPickedUp');
            $m->addField('holyShitFrags');
            $m->addField('holyShitFragged');
            $m->addField('carrierFrags');
            $m->addField('carrierFragsRecv');
        });
    }


    /**
     * @return wk\m\Action
     */
    static function weapon_usage()
    {
        return self::instance(__FUNCTION__, function(&$m)
        {
            $m = new wk\m\Action(Storage::main(), 'weapon_usage');
            $m->pkey = ['game_id', 'guid'];
            $m->setCache(Storage::cache());

            $m->addField('weap');
            $m->addField('shots');
        });
    }
}
