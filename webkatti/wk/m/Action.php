<?php

namespace wk\m;



class Action extends \afw\m\Model
{

    function countByGuid()
    {
        return $this->db()
            ->fields('guid, sum(count) as count')
            ->groupBy('guid');
    }

}
