<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;

use afw\APDO;
use afw\Session;



class User extends Model
{

    const SESSION_DATA = 'afw\m\User::data';

    /**
     * @var Supervisor
     */
    protected $supervisor;
    protected $data;

    public $fieldName = 'name';
    public $fieldPassword = 'password';



    function __construct(APDO $db, $table, Supervisor $supervisor = null)
    {
        parent::__construct($db, $table);
        $this->supervisor = $supervisor;
        $this->data = Session::get(self::SESSION_DATA);
    }



    function authenticate($name, $password)
    {
        if (isset($this->supervisor))
        {
            $this->supervisor->authenticate($name, $password);
        }

        $values = $this->filterValues([
            $this->fieldName => $name,
            $this->fieldPassword => $password,
        ]);

        $this->data = $this->db()
            ->where(
                "$this->fieldName=? AND $this->fieldPassword=?",
                [$values[$this->fieldName], $values[$this->fieldPassword]]
            )
            ->one();

        Session::set(self::SESSION_DATA, $this->data);

        return $this->data;
    }



    function isAuthed()
    {
        return !empty($this->data[$this->pkey]);
    }



    function getData($name = null)
    {
        if (isset($this->data))
        {
            if (isset($name))
            {
                if (isset($this->data[$name]))
                {
                    return $this->data[$name];
                }
            }
            else
            {
                return $this->data;
            }
        }
        return null;
    }



    function getPkey()
    {
        return isset($this->data) ? $this->data[$this->pkey] : null;
    }



    function isSupervisor()
    {
        return isset($this->supervisor) ? $this->supervisor->isAuthed() : false;
    }



    /**
     * @return Supervisor
     */
    function supervisor()
    {
        return $this->supervisor;
    }



    function update(array $rawValues, $id, array $oldValues = null, &$newValues = null)
    {
        if (empty($rawValues[$this->fieldPassword]))
        {
            unset($rawValues[$this->fieldPassword]);
        }
        parent::update($rawValues, $id, $oldValues, $newValues);
    }

}
