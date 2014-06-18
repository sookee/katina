<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;

use \PDO;



class APDO
{

    const DEFAULT_PKEY_NAME = 'id';

    public $pdo;

    private $dsn;
    private $username;
    private $password;
    private $options;

    /**
     * @var ILog
     */
    private $log;

    /**
     * @var ICache
     */
    private $cache;

    private $queryCount;
    private $cachedCount;

    private $nothing = false;
    private $table;
    private $pkey = self::DEFAULT_PKEY_NAME;
    private $where;
    private $groupby;
    private $having;
    private $orderby;
    private $limit;
    private $offset = 0;
    private $fields = '*';
    private $args = [];
    private $callbacks = [];

    private $lastCache;
    private $lastTable;
    private $lastWhere;
    private $lastArgs;
    private $lastLimit;
    private $lastOffset;

    private $lastRowCount;
    private $lastQuery;



    function __construct($dsn, $username, $password, $options)
    {
        $this->dsn = $dsn;
        $this->username = $username;
        $this->password = $password;
        $this->options = $options;
    }



    /**
     * @return \PDO
     */
    function pdo()
    {
        if (!isset($this->pdo))
        {
            $this->options = (array)$this->options + [
                PDO::ATTR_ERRMODE           => PDO::ERRMODE_EXCEPTION,
                PDO::ATTR_EMULATE_PREPARES  => false,
            ];
            $this->pdo = new PDO($this->dsn, $this->username, $this->password, $this->options);
            $this->queryCount = 0;
        }
        return $this->pdo;
    }



    function connected()
    {
        return isset($this->pdo);
    }



    function setLog(ILog $log)
    {
        $this->log = $log;
    }



    private function log($message, $args)
    {
        if (isset($this->log))
        {
            $this->log->add($message);
            $this->log->add(print_r($args, true));
        }
    }



    private function cacheKeyStatement($statement, $args, $fetch_style)
    {
        return 'st.' . md5($statement . '-args-' . implode('-', $args) . '-fs-' . $fetch_style);
    }



    private function cacheKeyRow($id)
    {
        return 'id.' . md5($this->table . '-fields-' . $this->fields . '-id-' . $id);
    }



    private function cacheSetStatement($statement, $args, $fetch_style, $result)
    {
        if (isset($this->cache))
        {
            $this->cache->set($this->cacheKeyStatement($statement, $args, $fetch_style), $result);

            // references can't use complex keys
            if (!empty($result) && !is_array($this->pkey))
            {
                if (isset($result[$this->pkey]))
                {
                    $this->cache->set($this->cacheKeyRow($result[$this->pkey]), $result);
                }
                else
                {
                    foreach ($result as $row)
                    {
                        if (isset($row[$this->pkey]))
                        {
                            $this->cache->set($this->cacheKeyRow($row[$this->pkey]), $row);
                        }
                    }
                }
            }
        }
    }



    private function cacheGetStatement($statement, $args, $fetch_style)
    {
        return isset($this->cache)
            ? $this->cache->get($this->cacheKeyStatement($statement, $args, $fetch_style))
            : null;
    }



    private function cacheGetRow($id)
    {
        return isset($this->cache)
            ? $this->cache->get($this->cacheKeyRow($id))
            : null;
    }



    function cacheClear()
    {
        if (isset($this->cache))
        {
            $this->cache->clear();
        }
    }



    function queryCount()
    {
        return $this->queryCount;
    }



    function cachedCount()
    {
        return $this->cachedCount;
    }



    function lastCache()
    {
        return $this->lastCache;
    }



    function lastTable()
    {
        return $this->lastTable;
    }



    function lastWhere()
    {
        return $this->lastWhere;
    }



    function lastArgs()
    {
        return $this->lastArgs;
    }



    function lastLimit()
    {
        return $this->lastLimit;
    }



    function lastOffset()
    {
        return $this->lastOffset;
    }



    function lastRowCount()
    {
        return $this->lastRowCount;
    }



    function lastQuery()
    {
        return $this->lastQuery;
    }



    function currTable()
    {
        return $this->table;
    }



    function currWhere()
    {
        return $this->where;
    }



    function currArgs()
    {
        return $this->args;
    }



    function currLimit()
    {
        return $this->limit;
    }



    function currOffset()
    {
        return $this->offset;
    }



    function execute($statement, $args = null)
    {
        $this->log($statement, $args);
        $sth = $this->pdo()->prepare($statement);
        $result = $sth->execute((array)$args);
        ++$this->queryCount;
        $this->lastRowCount = $sth->rowCount();
        $sth->closeCursor();
        $this->lastQuery = $statement;
        $this->cacheClear();

        return $result;
    }



    function select($statement, $args = null, $fetch_style = PDO::FETCH_ASSOC)
    {
        $args = (array)$args;

        $result = $this->cacheGetStatement($statement, $args, $fetch_style);
        if (isset($result))
        {
            ++$this->cachedCount;
        }
        else
        {
            $this->log($statement, $args);
            $sth = $this->pdo()->prepare($statement);
            $sth->execute($args);
            ++$this->queryCount;
            $result = $sth->fetchAll($fetch_style);
            $sth->closeCursor();

            $this->cacheSetStatement($statement, $args, $fetch_style, $result);
        }

        $this->lastRowCount = empty($result) ? 0 : count($result);
        $this->lastQuery = $statement;

        return $result;
    }



    function selectK($statement, $args = null)
    {
        return $this->select($statement, $args, PDO::FETCH_KEY_PAIR);
    }



    function selectOne($statement, $args = null, $fetch_style = PDO::FETCH_ASSOC)
    {
        $result = $this->select($statement, $args, $fetch_style);
        return empty($result) ? null : $result[0];
    }



    function selectOneL($statement, $args = null)
    {
        return $this->selectOne($statement, $args, PDO::FETCH_NUM);
    }



    function reset()
    {
        $this->lastCache = $this->cache;
        $this->lastTable = $this->table;
        $this->lastWhere = $this->where;
        $this->lastArgs = $this->args;
        $this->lastLimit = $this->limit;
        $this->lastOffset = $this->offset;

        $this->cache = null;
        $this->nothing = false;
        $this->table = null;
        $this->pkey = self::DEFAULT_PKEY_NAME;
        $this->where = null;
        $this->groupby = null;
        $this->having = null;
        $this->orderby = null;
        $this->limit = null;
        $this->offset = 0;
        $this->fields = '*';
        $this->args = [];
        $this->callbacks = [];
    }



    function cache(ICache $cache = null)
    {
        $this->cache = $cache;
        return $this;
    }



    function nothing()
    {
        $this->nothing = true;
        return $this;
    }



    function table($table)
    {
        $this->table = $table;
        return $this;
    }



    function from($table)
    {
        return $this->table($table);
    }



    function in($table)
    {
        return $this->table($table);
    }



    function pkey($name)
    {
        $this->pkey = $name;
        return $this;
    }



    function join($table, $on = null, $args = [], $joinType = '')
    {
        $this->table .= "\n$joinType JOIN " . $table;
        if (!empty($on))
        {
            $this->table .= ' ON (' . $on . ')';
            $this->args = array_merge($this->args, array_values((array)$args));
        }
        return $this;
    }



    function where($where, $args = [], $or = false)
    {
        if (empty($this->where))
        {
            $this->where = $where;
        }
        else
        {
            $this->where
                = '(' . $this->where . ')'
                . ($or ? ' OR ' : ' AND ')
                . '(' . $where . ')';
        }
        $this->where;
        $this->args = array_merge($this->args, array_values((array)$args));
        return $this;
    }



    function orWhere($where, $args = [])
    {
        return $this->where($where, $args, true);
    }



    function key($args, $name = null, $or = false)
    {
        if (empty($name))
        {
            $name = $this->pkey;
        }
        if (is_array($name))
        {
            foreach ($name as $i => $n)
            {
                $this->key($args[$i], $n, $or);
            }
            return $this;
        }
        if (is_array($args))
        {
            $where = $name . ' IN (' . implode(',', array_fill(0, count($args), '?')) . ')';
        }
        else
        {
            $args = [$args];
            $where = $name . '=?';
        }
        return $this->where($where, $args, $or);
    }



    function orKey($args, $name = null)
    {
        return $this->key($args, $name, true);
    }



    function groupBy($groupby)
    {
        $this->groupby = $groupby;
        return $this;
    }



    function having($having, $args = [])
    {
        $this->having = $having;
        $this->args = array_merge($this->args, array_values((array)$args));
        return $this;
    }



    function orderBy($orderby)
    {
        $this->orderby = $orderby;
        return $this;
    }



    function addOrderBy($field, $desc = false)
    {
        if (!empty($this->orderby))
        {
            $this->orderby .= ', ';
        }
        $this->orderby .= $field;
        if ($desc)
        {
            $this->orderby .= ' DESC';
        }
        return $this;
    }



    function limit($limit)
    {
        $this->limit = $limit;
        return $this;
    }



    function offset($offset)
    {
        $this->offset = $offset;
        return $this;
    }



    function fields($fields)
    {
        $this->fields = implode(', ', (array)$fields);
        return $this;
    }



    function handler($callback)
    {
        $this->callbacks [] = $callback;
        return $this;
    }



    private function handlers($result)
    {
        foreach ($this->callbacks as $callback)
        {
            $result = $callback($result);
        }
        $this->reset();
        return $result;
    }



    function buildSelect()
    {
        $statement = 'SELECT ' . $this->fields
            . "\nFROM " . $this->table
            . (!empty($this->where)     ? "\nWHERE "    . $this->where : '')
            . (!empty($this->groupby)   ? "\nGROUP BY " . $this->groupby : '')
            . (!empty($this->having)    ? "\nHAVING "   . $this->having : '')
            . (!empty($this->orderby)   ? "\nORDER BY " . $this->orderby : '')
            . (!empty($this->limit)     ? "\nLIMIT "    . $this->offset . ', ' . $this->limit : '');
        return $statement;
    }



    function all($fetch_style = PDO::FETCH_ASSOC)
    {
        try
        {
            return $this->handlers(
                $this->nothing
                    ? []
                    : $this->select($this->buildSelect(), $this->args, $fetch_style)
            );
        }
        catch (\Exception $e)
        {
            $this->reset();
            throw $e;
        }
    }



    function allK()
    {
        return $this->all(PDO::FETCH_KEY_PAIR);
    }



    function one($fetch_style = PDO::FETCH_ASSOC)
    {
        $r = $this
            ->limit(1)
            ->all($fetch_style);
        return empty($r) ? null : $r[0];
    }



    function oneL()
    {
        return $this->one(PDO::FETCH_NUM);
    }



    function page($page = 1, $fetch_style = PDO::FETCH_ASSOC)
    {
        return $this
            ->offset($this->limit * (($page ? : 1) - 1))
            ->all($fetch_style);
    }



    function pageK($page = 1)
    {
        return $this->page($page, PDO::FETCH_KEY_PAIR);
    }



    function keys($name = null)
    {
        if (empty($name))
        {
            $name = $this->pkey;
        }
        return $this
            ->fields([$name, $name])
            ->allK();
    }



    function count()
    {
        if (!isset($this->cache))
        {
            $this->cache = $this->lastCache;
        }
        $where = $this->where ? : $this->lastWhere;
        $statement = "SELECT COUNT(*)\nFROM " . ($this->table ? : $this->lastTable)
            . (!empty($where) ? "\nWHERE " . $where : '');

        list($count) = $this->selectOneL($statement, $this->args ? : $this->lastArgs);
        $this->reset();
        return $count;
    }



    function insert(array $values)
    {
        if (!isset($values[0]))
        {
            $values = [$values];
        }

        $names = array_keys($values[0]);
        $this->args = [];
        foreach ($values as $v)
        {
            foreach ($v as $a)
            {
                $this->args [] = $a;
            }
        }

        $statement = 'INSERT INTO ' . $this->table . ' ('
            . implode(',', $names) . ") VALUES\n("
            . implode(
                "),\n(",
                array_fill(0, count($values), implode(',', array_fill(0, count($names), '?')))
            ) . ')';

        try
        {
            $this->execute($statement, $this->args);
        }
        catch (\Exception $e)
        {
            $this->reset();
            throw $e;
        }

        $this->reset();
        return $this->pdo->lastInsertId();
    }



    function update(array $values)
    {
        if (empty($values))
        {
            return true;
        }

        $this->args = array_merge(array_values($values), $this->args);

        $statement = 'UPDATE ' . $this->table . "\nSET\n\t"
            . implode("=?,\n\t", array_keys($values)) . '=?'
            . (!empty($this->where) ? "\nWHERE " . $this->where : '');

        try
        {
            $r = $this->execute($statement, $this->args);
        }
        catch (\Exception $e)
        {
            $this->reset();
            throw $e;
        }

        $this->reset();
        return $r;
    }



    function delete()
    {
        $statement = 'DELETE FROM ' . $this->table
            . (!empty($this->where) ? "\nWHERE " . $this->where : '');

        try
        {
            $r = $this->execute($statement, $this->args);
        }
        catch (\Exception $e)
        {
            $this->reset();
            throw $e;
        }

        $this->reset();
        return $r;
    }



    /**
     * get parent
     */
    function referrers(&$data, $referrerName, $referenceName, $key = null, $pkey = null)
    {
        if (empty($data))
        {
            return $this->nothing();
        }
        if (empty($key))
        {
            $key = $referenceName;
        }
        if (empty($pkey))
        {
            $pkey = $this->pkey;
        }
        foreach ($data as $k => $v)
        {
            $isArray = is_int($k);
            break;
        }

        $index = [];
        $cached = [];
        $keys = [];
        if ($isArray)
        {
            foreach ($data as &$item)
            {
                $k = $item[$key];
                if (isset($k))
                {
                    $index[$k] [] = & $item;
                    if (empty($cached[$k]) && empty($keys[$k]))
                    {
                        $cache = $this->cacheGetRow($k);
                        if (isset($cache))
                        {
                            $cached[$k] = $cache;
                        }
                        else
                        {
                            $keys[$k] = $k;
                        }
                    }
                }
            }
            unset($item);
        }
        else
        {
            $k = $data[$key];
            if (isset($k))
            {
                $index[$k] [] = & $data;
                if (empty($cached[$k]) && empty($keys[$k]))
                {
                    $cache = $this->cacheGetRow($k);
                    if (isset($cache))
                    {
                        $cached[$k] = $cache;
                    }
                    else
                    {
                        $keys[$k] = $k;
                    }
                }
            }
        }
        if (empty($keys))
        {
            $this->nothing();
        }
        else
        {
            $this->key($keys);
        }

        return $this
            ->handler(function ($result) use ($index, $cached, $referrerName, $referenceName, $pkey)
            {
                $r = [];

                if (isset($cached))
                {
                    foreach ($cached as &$row)
                    {
                        $r [] = & $row;
                        foreach ($index[$row[$pkey]] as &$item)
                        {
                            $item[$referenceName] = & $row;
                            $row[$referrerName] [] = & $item;
                        }
                        unset($item);
                    }
                    unset($row);
                }
                if (isset($result))
                {
                    foreach ($result as &$row)
                    {
                        $r [] = & $row;
                        foreach ($index[$row[$pkey]] as &$item)
                        {
                            $item[$referenceName] = & $row;
                            $row[$referrerName] [] = & $item;
                        }
                        unset($item);
                    }
                    unset($row);
                }

                return $r;
            });
    }



    /**
     * get children
     */
    function references(&$data, $referrerName, $referenceName, $key = null, $pkey = null)
    {
        unset($data[$referrerName]);

        if (empty($data))
        {
            return $this->nothing();
        }
        if (empty($key))
        {
            $key = $referenceName;
        }
        if (empty($pkey))
        {
            $pkey = $this->pkey;
        }
        foreach ($data as $k => $v)
        {
            $isArray = is_int($k);
            break;
        }

        $index = [];
        if ($isArray)
        {
            foreach ($data as &$item)
            {
                $index[$item[$pkey]] = & $item;
            }
            unset($item);
        }
        else
        {
            $index[$data[$pkey]] = & $data;
        }

        return $this
            ->key(array_keys($index), $key)
            ->handler(function ($result) use ($index, $referrerName, $referenceName, $key)
            {
                if (empty($result))
                {
                    return [];
                }

                $r = [];
                foreach ($result as &$row)
                {
                    $r [] = & $row;
                    $item = & $index[$row[$key]];
                    $item[$referrerName] [] = & $row;
                    $row[$referenceName] = & $item;
                }
                unset($item);
                unset($row);

                return $r;
            });
    }

}
