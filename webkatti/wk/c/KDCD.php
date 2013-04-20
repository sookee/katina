<?php

namespace wk\c;

use M;



class KDCD extends \afw\c\Controller
{

    public $kills;
    public $caps;
    public $deaths;
    public $players;
    public $minDeaths;


    function __construct($kills, $caps, $deaths, $minDeaths = 1)
    {
        $this->setTemplate(__CLASS__);
        $this->kills        = $kills;
        $this->caps         = $caps;
        $this->deaths       = $deaths;
        $this->minDeaths    = $minDeaths;
    }



    public function render()
    {
        $this->players = [];

        foreach ($this->deaths as $guid => $count)
        {
            if ($count >= $this->minDeaths)
            {
                $this->players[$guid] = [
                    'deaths'    => $count,
                    'kills'     => (int)@$this->kills[$guid]    ? : '',
                    'caps'      => (int)@$this->caps[$guid]     ? : '',
                ];

            }
        }

        if (!empty($this->players))
        {
            M::user()->setNamesByGuid($this->players);

            foreach ($this->players as $guid => &$player)
            {
                $player['kd']   = $player['kills'] / $player['deaths'];
                $player['cd']   = $player['caps'] / $player['deaths'];

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
            }
            unset($player);
        }

        parent::render();
    }


}
