<?php

namespace wk;

use Storage;



class Utils
{

    static function colorecho($s)
    {
        if (empty($s))
        {
            return;
        }

        $s = htmlspecialchars($s);
        $parts = explode('^', $s);
        for ($i=0; $i<count($parts); $i++)
        {
            if ($i == 0)
            {
                echo '<span class="w">', $parts[$i];
            }
            else
            {
                switch (@$parts[$i][0])
                {
                    case 1: echo '</span><span class="r">', substr($parts[$i], 1); break;
                    case 2: echo '</span><span class="g">', substr($parts[$i], 1); break;
                    case 3: echo '</span><span class="y">', substr($parts[$i], 1); break;
                    case 4: echo '</span><span class="b">', substr($parts[$i], 1); break;
                    case 5: echo '</span><span class="c">', substr($parts[$i], 1); break;
                    case 6: echo '</span><span class="m">', substr($parts[$i], 1); break;
                    case 7: echo '</span><span class="w">', substr($parts[$i], 1); break;
                    case '0': case 8: echo '</span><span class="o">', substr($parts[$i], 1); break;
                    default: echo '^', $parts[$i]; break;
                }
            }
        }

        echo '</span>';
    }



    static function uncolor($s)
    {
        return preg_replace('`\^[0-8]`', '', $s);
    }



    static $weapons = [
        0 => 'Unknown',
        1 => 'Shotgun',
        2 => 'Gauntlet',
        3 => 'Machinegun',
        4 => 'Grenade',
        5 => 'Grenade splash',
        6 => 'Rocket',
        7 => 'Rocket splash',
        8 => 'Plasma',
        9 => 'Plasma splash',
        10 => 'Railgun',
        11 => 'Lightning',
        12 => 'BFG',
        13 => 'BFG splash',
        14 => 'Water',
        15 => 'Slime',
        16 => 'Lava',
        17 => 'Crush',
        18 => 'Telefrag',
        19 => 'Falling',
        20 => 'Suicide',
        21 => 'Target laser',
        22 => 'Trigger hurt',
        23 => 'Nail',
        24 => 'Chaingun',
        25 => 'Proximity mine',
        26 => 'Kamikaze',
        27 => 'Juiced',
        28 => 'Grapple',
    ];



    private static $servers;

    static function server($host, $port = null)
    {
        if (isset($port))
        {
            $host .= ':' . $port;
        }

        if (!isset(self::$servers))
        {
            self::$servers = Storage::cache()->get('dpmaster_servers');
            if (!isset(self::$servers))
            {
                $dpmaster_file = 'dpmaster.xml';
                if (@filemtime($dpmaster_file) < time() - 60 * 60 * 24)
                {
                    file_put_contents(
                        $dpmaster_file,
                        file_get_contents('http://dpmaster.deathmask.net/?game=openarena&xml=1&showall=1&xmlcarets=1')
                    );
                }

                $dpmaster = simplexml_load_file($dpmaster_file);
                self::$servers = ['server' => '[server]'];
                foreach ($dpmaster->server as $server)
                {
                    if (!empty($server->hostname))
                    {
                        self::$servers[(string)$server->hostname] = htmlspecialchars_decode(@$server->name ? : $server->hostname);
                    }
                }
                Storage::cache()->set('dpmaster_servers', self::$servers);
            }
        }

        if (isset(self::$servers[$host]))
        {
            self::colorecho(self::$servers[$host]);
        }
        else
        {
            echo $host;
        }
    }



	static function formatTimeHMS($time)
	{
        if (empty($time))
        {
            return '';
        }
        $s = $time % 60;
        $time = ($time - $s) / 60;
        $m = $time % 60;
        $h = ($time - $m) / 60;
        return sprintf('%02d:%02d:%02d', $h, $m, $s);
	}



	static function formatTimeMS($time)
	{
        if (empty($time))
        {
            return '';
        }
        $s = $time % 60;
        $m = ($time - $s) / 60;
        return sprintf('%02d:%02d', $m, $s);
	}

}
