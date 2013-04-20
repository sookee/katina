<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;



class Statistics extends Model
{

	/**
	 * @var Statistics[]
	 */
	protected static $instances = [];
	protected static $instancesCount = 0;
	protected $instanceId;

	public $dkey = 'd';
	public $period = 'Y-m-d';
	public $multiplier = 1;
	/**
	 * @var \afw\APDO
	 */
	protected $db;
	protected $table;
	protected $params = [];
	protected $counters = [];
	protected $filters = [];
	protected $isNew;
	protected $incremented = false;



	function __construct(\afw\APDO $db, $table, $defaultCounter = 'all_count')
	{
		$this->db = $db;
		$this->table = $table;
		$this->addCounter($defaultCounter);

		$this->instanceId = self::$instancesCount++;
		self::$instances[$this->instanceId] = $this;
	}



	static function flush()
	{
		foreach (self::$instances as $instance)
		{
			if (!$instance->incremented)
			{
				$instance->inc();
			}
		}
	}



	/**
	 * @param callback|bool $filter
	 * @return StatisticsFilter
	 */
	static function filter($filter)
	{
		if (!is_callable($filter))
		{
			$filter = function() use ($filter)
			{
				return (bool)$filter;
			};
		}
		return new StatisticsFilter($filter);
	}



	/**
	 * @return StatisticsFilter
	 */
	static function unique(self $statistics)
	{
		return new StatisticsFilter(function() use ($statistics)
		{
			return $statistics->isNew();
		});
	}



	function setParam($name, $value)
	{
		$this->params[$name] = $value;
		return $this;
	}



	function linkParam($name, &$value)
	{
		$this->params[$name] =& $value;
		return $this;
	}



	function addCounter($name, StatisticsFilter $filter = null)
	{
		$this->counters[$name] = null;

		$args = func_get_args();
		$args_num = func_num_args();
		for ($i = 1; $i < $args_num; $i++)
		{
			$this->filters[$name] []= $args[$i];
		}

		return $this;
	}



	function incCounter($name, $times = 1)
	{
		$this->counters[$name] += $times * $this->multiplier;
	}



	/**
	 * @return APDO
	 */
	function db()
	{
		return parent::db()
			->addOrderBy($this->dkey, true);
	}



	function get($params = null, $null = null)
	{
		return parent::get($params);
	}



	function tryGet($params = null, $null = null)
	{
		if (!isset($params))
		{
			$params = $this->params;
		}
		foreach ($params as $name=>$value)
		{
			$this->db->key($value, $name);
		}
		return parent::tryGet(date($this->period), $this->dkey);
	}



	function exists($params = null)
	{
		$this->db->fields('1');
		$r = $this->get($params);
		return !empty($r);
	}



	function isNew()
	{
		if (!isset($this->isNew))
		{
			$this->isNew = !$this->exists();
		}
		return $this->isNew;
	}



	function inc()
	{
		$this->incremented = true;
		$this->incUndefinedCounters();

		$args = [];
		$update_values = '';
		$insert_names = '';
		$insert_values = '';
		foreach ($this->counters as $name=>$value)
		{
			if ($value > 0)
			{
				$update_values .= ",$name=$name+?";
				$args []= $value;
				$insert_names .= ','.$name;
				$insert_values .= ',?';
			}
		}

		$conditions = 'WHERE '.$this->dkey.'=?';
		$args []= date($this->period);
		$insert_names .= ','.$this->dkey;
		$insert_values .= ',?';

		foreach ($this->params as $name=>$value)
		{
			$conditions .= " AND $name=?";
			$args []= $value;
			$insert_names .= ','.$name;
			$insert_values .= ',?';
		}

		if (empty($update_values))
		{
			return;
		}
		$update_values[0] = ' ';

		if (!$this->isNew)
		{
			$this->db->execute("UPDATE $this->table SET $update_values $conditions", $args);
			if (isset($this->isNew))
			{
				return;
			}
			$this->isNew = $this->db->lastRowCount() == 0;
			if (!$this->isNew)
			{
				return;
			}
		}

		$insert_names[0] = ' ';
		$insert_values[0] = ' ';
		$this->db->execute("INSERT INTO $this->table ($insert_names) VALUES ($insert_values)", $args);

		return;
	}



	protected function incUndefinedCounters()
	{
		foreach ($this->counters as $name=>$value)
		{
			if (!isset($value))
			{
				if (!$this->checkFilters($name))
				{
					continue;
				}
				$this->incCounter($name);
			}
		}
	}



	protected function checkFilters($name)
	{
		if (isset($this->filters[$name]))
		{
			foreach ($this->filters[$name] as $filter)
			{
				if ($filter instanceof StatisticsFilter && !$filter->check())
				{
					return false;
				}
			}
		}
		return true;
	}

}



class StatisticsFilter
{

	protected $callback;
	protected $result;

	function __construct($callback)
	{
		$this->callback = $callback;
	}



	function check()
	{
		if (!isset($this->result))
		{
			$callback = $this->callback;
			return $this->result = $callback();
		}
		return $this->result;
	}

}
