<?php

namespace wk\c;

use M;



class PlayerStats extends \afw\c\Controller
{

    public $kills;
    public $caps;
    public $deaths;
    public $time;
    public $players;
    public $minDeaths;
    public $minTime;


    function __construct($kills, $caps, $deaths, $time, $minDeaths = 1, $minTime = 10)
    {
        $this->setTemplate(__CLASS__);
        $this->kills        = $kills;
        $this->caps         = $caps;
        $this->deaths       = $deaths;
        $this->time         = $time;
        $this->minDeaths    = $minDeaths;
        $this->minTime      = $minTime;
    }



    public function render()
    {
        $this->players = [];

        foreach ($this->deaths as $guid => $count)
        {
            if ($count >= $this->minDeaths && @$this->time[$guid] >= $this->minTime)
            {
                $this->players[$guid] = [
                    'deaths'    => $count,
                    'time'      => (int)@$this->time[$guid]     ? : '',
                    'kills'     => (int)@$this->kills[$guid]    ? : '',
                    'caps'      => (int)@$this->caps[$guid]     ? : '',
                ];

            }
        }

        if (!empty($this->players))
        {
            M::user()->setNamesByGuid($this->players);

            foreach ($this->players as &$player)
            {
                $player['kd']   = $player['kills'] / $player['deaths'];
                $player['cd']   = $player['caps'] / $player['deaths'];
                $player['tk']   = empty($player['kills']) ? null : $player['time'] / $player['kills'];
                $player['ct']   = $player['caps'] / ($player['time'] / 3600);

                if (empty($player['kd']))
                {
                    $player['kdcd'] = $player['cd'] * 0.001;
                }
                else if (empty($player['cd']))
                {
                    $player['kdcd'] = $player['kd'] * 0.001;
                }
                else
                {
                    $player['kdcd'] = $player['kd'] * $player['cd'];
                }
            }
            unset($player);

            uasort($this->players, function($a, $b)
            {
                return ($b['kdcd'] - $a['kdcd']) * 1000000;
            });

            foreach ($this->players as &$player)
            {
                $player['kd']   = round($player['kd'] * 100)    ? : '';
                $player['cd']   = round($player['cd'] * 100)    ? : '';
                $player['kdcd'] = round($player['kdcd'] * 100)  ? : '';

                $player['time'] = \wk\Utils::formatTimeHMS($player['time']);
                $player['tk']   = sprintf('%.1f', $player['tk']) ? : '';
                $player['ct']   = round($player['ct']) ? : '';
            }
            unset($player);
        }

        parent::render();
    }


}