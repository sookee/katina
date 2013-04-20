<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;



class Map extends Model
{

    public $fieldParentId   = 'pid';
    public $fieldTitle      = 'title';
    public $fieldDeep       = 'deep';
    public $fieldParent     = 'parent';
    public $fieldChilds     = 'childs';
    public $flatPadding     = ' &nbsp; ';

	protected $index;
	protected $map;
	protected $simpleMap;
	protected $flatMap;



	protected function getChildsFromIndex(array &$list, $deep = 0, $id = 0, $parent = null)
	{
		$childs = [];
		$simpleChilds = [];

		foreach ($list as $v)
        {
            if (!isset($this->index[$v[$this->pkey]]))
            {
                if ((int)$v[$this->fieldParentId] == $id)
                {
                    $v[$this->fieldParent] = $parent;
                    $v[$this->fieldDeep] = $deep;
                    $childs[$v[$this->pkey]] = $v;
                    $this->index[$v[$this->pkey]] =& $childs[$v[$this->pkey]];
                    list($childs[$v[$this->pkey]][$this->fieldChilds], $subSimpleChilds) =
                        $this->getChildsFromIndex($list, $deep + 1, $v[$this->pkey], $childs[$v[$this->pkey]]);
                    if (empty($subSimpleChilds))
                    {
                        $simpleChilds[$v[$this->pkey]] = $v[$this->fieldTitle];
                    }
                    else
                    {
                        $simpleChilds[$v[$this->fieldTitle]] = $subSimpleChilds;
                    }
                }
            }
        }

		return [$childs, $simpleChilds];
	}



	protected function createMap()
	{
		if (!isset($this->index))
		{
            $this->index = [];
			$list = $this->db()->all();
			list($this->map, $this->simpleMap) = $this->getChildsFromIndex($list);
		}
	}



    function index()
	{
		$this->createMap();
		return $this->index;
	}



    function item($id)
    {
        $this->createMap();
        return @$this->index[$id];
    }



	function map()
	{
		$this->createMap();
		return $this->map;
	}



	function simpleMap()
	{
		$this->createMap();
		return $this->simpleMap;
	}



    function flatMap()
    {
        if (!isset($this->flatMap))
		{
            foreach ($this->map() as $item)
            {
                $this->flatMapRecurse($item);
            }
        }
		return $this->flatMap;
    }



    private function flatMapRecurse($item)
    {
        $this->flatMap[$item[$this->pkey]]
            = str_repeat($this->flatPadding, $item[$this->fieldDeep])
            . $item[$this->fieldTitle];
        if (!empty($item[$this->fieldChilds]))
        {
            foreach ($item[$this->fieldChilds] as $child)
            {
                $this->flatMapRecurse($child);
            }
        }
    }

}