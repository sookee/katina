<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;

use afw\HttpException;
use afw\APDO;
use afw\File;



class Model extends ModelWithFields
{

	/**
	 * @var APDO
	 */
	protected $db;
	protected $table;
	public $references = [];
	public $referrers = [];



	function __construct(APDO $db, $table)
	{
		$this->db = $db;
		$this->table = $table;
	}



    function getTitle()
    {
        return $this->title ?: $this->table;
    }



    function getTable()
    {
        return $this->table;
    }



	/**
	 * @return APDO
	 */
	function db()
	{
		return $this->_db();
	}



	/**
	 * @return APDO
	 */
	function _db()
	{
		return $this->db
			->table($this->table)
			->pkey($this->pkey)
            ->cache($this->cache);
	}



	function insert(array $rawValues, &$newValues = null)
	{
        $newValues = $this->filterValues($rawValues);
		$id = $this->_db()
			->insert($newValues);
		$this->insertFiles($id, $rawValues, $newValues);
		return $id;
	}



	function update(array $rawValues, $id, array $oldValues = null, &$newValues = null)
	{
        $newValues = $this->filterValues($rawValues);
		$r = $this->_db()
            ->key($id)
			->update($newValues);
		$this->updateFiles($id, $rawValues, $newValues, $oldValues);
		return $r;
	}



	function delete($id, array $values = [], array $oldValues = null)
	{
		$r = $this->_db()
            ->key($id)
			->delete();
		$this->deleteFiles($id, $oldValues);
		return $r;
	}



	function get($id, $key = null)
	{
		$r = $this->tryGet($id, $key);

		if (empty($r))
		{
			throw new HttpException(404);
		}

		return $r;
	}



	function tryGet($id, $key = null)
	{
		if (!isset($id) || $id === [])
		{
			return null;
		}
		return $this->db()
			->key($id, $key)
			->first();
	}



	function pages()
	{
        $this->checkPage();
		return ceil($this->db->count() / max($this->db->lastLimit(), 1));
	}



    function checkPage()
    {
		if ($this->db->lastOffset() > 0 && $this->db->lastRowCount() == 0)
		{
			throw new HttpException(404);
		}
    }



    function fieldName($name)
    {
        return $this->table.'.'.$name;
    }



	function dest($name, $data = null, $index = 0, $id = null)
	{
		return empty($data[$name])
            ? null
            : \afw\File::dest($this->getField($name)->fileParams(), $index, $this->destArgs($name, $data, $id));
	}



    protected function destArgs($name, $data, $id = null)
    {
        if (isset($id))
        {
            $args = (array)$id;
        }
        else
        {
            $args = [];
            foreach ((array)$this->pkey as $pkey)
            {
                $args []= $data[$pkey];
            }
        }
        foreach ($args as &$arg)
        {
            $arg = implode('/', str_split($arg, 2));
        }
        $args []= $data[$name];
        return $args;
    }



	protected function insertFiles($id, $rawValues, $newValues)
	{
		try
		{
			foreach ($this->fields as $name=>$field)
			{
				$params = $field->fileParams();
				if (!empty($params) && !empty($rawValues[$name]['tmp_name']))
				{
					File::receive($rawValues[$name]['tmp_name'], $params, $this->destArgs($name, $newValues, $id));
				}
			}
		}
		catch (\Exception $e)
		{
			$this->delete($id);
			throw $e;
		}
	}



	protected function updateFiles($id, $rawValues, $newValues, $oldValues)
	{
		foreach ($this->fields as $name=>$field)
		{
			$params = $field->fileParams();
			if (!empty($params))
			{
				if (!empty($rawValues[$name]['tmp_name']))
				{
                    if (!empty($oldValues[$name]) && $oldValues[$name] != $newValues[$name])
                    {
                        File::delete($params, $this->destArgs($name, $oldValues, $id));
                    }
					File::receive($rawValues[$name]['tmp_name'], $params, $this->destArgs($name, $newValues, $id));
				}
				else if (!empty($rawValues[$name.'-delete']) && isset($oldValues[$name]))
				{
					File::delete($params, $this->destArgs($name, $oldValues, $id));
				}
			}
		}
	}



	protected function deleteFiles($id, $oldValues)
	{
		foreach ($this->fields as $name=>$field)
		{
			$params = $field->fileParams();
			if (!empty($params) && isset($oldValues[$name]))
			{
				File::delete($params, $this->destArgs($name, $oldValues, $id));
			}
		}
	}



	/**
	 * @return Field
	 */
	function addReference(self $model, $referrerName, $referenceName, $key = null)
	{
		if (empty($key))
		{
			$key = $referenceName;
		}
		$this->references[$referenceName] = [$referrerName, $key, $model->pkey];
		$model->referrers[$referrerName] = [$referenceName, $key];

		return $this->addField($key);
	}



	/**
	 * get parent
     * @return APDO|array
	 */
	function referrers(&$data, $referrerName = null)
	{
        if (isset($referrerName))
        {
            list($referenceName, $key) = $this->referrers[$referrerName];
            return $this->db()
                ->referrers($data, $referrerName, $referenceName, $key);
        }
        else
        {
            $r = [];
            foreach ($this->referrers as $referrerName=>$buf)
            {
                list($referenceName, $key) = $buf;
                $r[$referrerName] = $this->db()
                    ->referrers($data, $referrerName, $referenceName, $key)
                    ->all();
            }
            return $r;
        }
	}



	/**
     * get children
     * @return APDO|array
     */
    function references(&$data, $referenceName = null)
	{
        if (isset($referenceName))
        {
            list($referrerName, $key, $pkey) = $this->references[$referenceName];
            return $this->db()
                ->references($data, $referrerName, $referenceName, $key, $pkey);
        }
        else
        {
            $r = [];
            foreach ($this->references as $referenceName=>$buf)
            {
                list($referrerName, $key) = $buf;
                $r[$referenceName] = $this->db()
                    ->references($data, $referrerName, $referenceName, $key)
                    ->all();
            }
            return $r;
        }
	}

}
