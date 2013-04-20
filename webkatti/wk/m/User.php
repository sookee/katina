<?php

namespace wk\m;



class User extends \afw\m\Model
{

    /**
     * @var \afw\m\Model
     */
    public $player;

    public function __construct(\afw\APDO $db, $table, \afw\m\Model $player)
    {
        parent::__construct($db, $table);
        $this->player = $player;
    }



    function setNamesByGuid(&$data)
    {
        $r = $this->db()
            ->fields('guid, name')
            ->key(array_keys($data), 'guid')
            ->allK();

        $notRegistered = [];

        foreach ($data as $guid => $d)
        {
            if (isset($r[$guid]))
            {
                $data[$guid]['name'] = $r[$guid];
                $data[$guid]['registered'] = true;
            }
            else
            {
                $notRegistered []= $guid;
            }
        }

        if (!empty($notRegistered))
        {
            $r = $this->player->db()
                ->fields('guid, name')
                ->key($notRegistered, 'guid')
                ->addOrderBy('date')
                ->addOrderBy('count')
                ->allK();

            foreach ($notRegistered as $guid)
            {
                $data[$guid]['name'] = @$r[$guid] ? : $guid;
            }
        }
    }



    function getNameByGuid($guid, &$registered)
    {
        $data = [$guid => []];
        $this->setNamesByGuid($data);
        $registered = @$data[$guid]['registered'];
        return $data[$guid]['name'];
    }

}
