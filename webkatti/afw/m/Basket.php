<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;

use afw\Session;



class Basket
{

    const SESSION_DATA = 'afw\m\Basket::data';

    public $funcCost;
    public $fieldAmount = 'amount';
    public $fieldPrice = 'price';
    public $fieldOptions = 'options';
    public $fieldChoice = 'choice';
    public $fieldChoiceKey = '__choice';

    /**
     * @var Model
     */
    protected $model;
    protected $data;



    function __construct(Model $itemModel, $funcCost = null)
    {
        $this->model = $itemModel;
        $this->funcCost = $funcCost ? : function($items, $fieldAmount, $fieldPrice, $fieldOptions, $fieldChoice)
        {
            $cost = 0;
            foreach ($items as $item)
            {
                $price = $item[$fieldPrice];
                if (!empty($item[$fieldChoice]))
                {
                    $options = (array)@unserialize($item[$fieldOptions]);
                    foreach ((array)@$item[$fieldChoice] as $name => $option)
                    {
                        $price += @$options[$name][$option];
                    }
                }
                $cost += $price * $item[$fieldAmount];
            }
            return $cost;
        };
    }



    protected function initData()
    {
        Session::read();
        if (!isset($_SESSION[self::SESSION_DATA]))
        {
            $_SESSION[self::SESSION_DATA] = [];
        }
        $this->data = & $_SESSION[self::SESSION_DATA];
    }



    protected function prepareOptions($options)
    {
        if (empty($options))
        {
            return [md5(serialize(null)), null];
        }
        if (is_array($options))
        {
            return [md5(serialize($options)), $options];
        }
        else
        {
            return [$options, null];
        }
    }



    function add($id = null, $amount = null, $options = null)
    {
        $id = $id ? : @$_POST[$this->model->pkey];
        $amount = $amount ? : (int)@$_POST[$this->fieldAmount] ? : 1;
        list($optionsKey, $o) = $this->prepareOptions($options ? : @$_POST[$this->fieldChoice]);
        if (empty($id))
        {
            return;
        }

        Session::start();
        $this->initData();

        if (isset($this->data[$id][$optionsKey]))
        {
            $this->data[$id][$optionsKey][0] += $amount;
            if ($this->data[$id][$optionsKey][0] <= 0)
            {
                unset($this->data[$id][$optionsKey]);
            }
        }
        else if ($amount > 0)
        {
            $this->data[$id][$optionsKey] = [$amount, $o];
        }

        Session::commit();
    }



    function set($id = null, $amount = null, $options = null)
    {
        $id = $id ? : @$_POST[$this->model->pkey];
        $amount = $amount ? : (int)@$_POST[$this->fieldAmount];
        list($optionsKey, $o) = $this->prepareOptions($options ? : @$_POST[$this->fieldChoice]);
        if (empty($id))
        {
            return;
        }

        Session::start();
        $this->initData();

        if ($amount > 0)
        {
            if ($options == $optionsKey)
            {
                $this->data[$id][$optionsKey][0] = $amount;
            }
            else
            {
                $this->data[$id][$optionsKey] = [$amount, $o];
            }
        }
        else
        {
            unset($this->data[$id][$optionsKey]);
        }

        Session::commit();
    }



    function setItems(array $items)
    {
        Session::start();
        $this->initData();

        foreach ((array)@$items as $id => $options)
        {
            foreach ($options as $optionsKey => $amount)
            {
                $this->set($id, $amount, $optionsKey);
            }
        }

        Session::commit();
    }



    function remove($id = null, $options = null)
    {
        $id = $id ? : @$_POST[$this->model->pkey];
        list($optionsKey, $options) = $this->prepareOptions($options ? : @$_POST[$this->fieldChoice]);
        if (empty($id))
        {
            return;
        }

        Session::start();
        $this->initData();

        unset($this->data[$id][$optionsKey]);

        Session::commit();
    }



    function clear()
    {
        Session::start();
        $this->initData();

        $this->data = [];

        Session::commit();
    }



    function getList(&$cost = null, &$amount = null)
    {
        $this->initData();
        if (empty($this->data))
        {
            return [];
        }

        $items = [];
        $buf = $this->model->db()
            ->key(array_keys($this->data))
            ->all();
        foreach ($buf as $row)
        {
            $items[$row[$this->model->pkey]] = $row;
        }

        $r = [];
        $amount = 0;
        foreach ($this->data as $id => $options)
        {
            foreach ($options as $optionsKey => $data)
            {
                list($a, $o) = $data;
                $row = $items[$id];
                $row[$this->fieldAmount] = $a;
                $row[$this->fieldChoice] = $o;
                $row[$this->fieldChoiceKey] = $optionsKey;
                $r [] = $row;
                $amount += $a;
            }
        }

        $cost = call_user_func(
            $this->funcCost,
            $r,
            $this->fieldAmount,
            $this->fieldPrice,
            $this->fieldOptions,
            $this->fieldChoice
        );

        return $r;
    }

}
