<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;

use afw\ICache;



class ModelFilterSkip extends \Exception {}



class ModelWithFields
{

    /**
     * @var ICache
     */
    protected $cache;
    protected $fieldsCreator = null;
    protected $title;

    /**
     * @var Field[]
     */
    public $fields = [];
    public $pkey = 'id';



    function setCache(ICache $cache = null)
    {
        $this->cache = $cache;
    }



    function setTitle($title)
    {
        $this->title = $title;
    }



    function getTitle()
    {
        return $this->title ? : get_class($this);
    }



    /**
     * @return Field
     */
    function addField($name)
    {
        return $this->fields[$name] = new Field($name);
    }



    /**
     * @return Field
     */
    function getField($name)
    {
        return @$this->fields[$name];
    }



    function dest($name, $index = 0)
    {
        return \afw\File::dest($this->getField($name)->fileParams(), $index);
    }



    function setFields($fieldsCreator)
    {
        $this->fields = null;
        $this->fieldsCreator = $fieldsCreator;
    }



    protected function filterValues(array $values)
    {
        if (!isset($this->fields) && isset($this->fieldsCreator))
        {
            $fieldsCreator = $this->fieldsCreator;
            $fieldsCreator($this);
        }

        if (!isset($this->fields))
        {
            return $values;
        }

        $result = [];
        $exceptions = [];

        $pkeys = array_flip((array)$this->pkey);

        foreach ($values as $name => $value)
        {
            if (isset($this->fields[$name]))
            {
                $field = $this->fields[$name];
                foreach ($field->getFilters() as $filter)
                {
                    try
                    {
                        $value = $filter($value, $name, $values, $result);
                    }
                    catch (ModelFilterSkip $e)
                    {
                        continue 2;
                    }
                    catch (\Exception $e)
                    {
                        $exceptions[$name] = $e;
                        break;
                    }
                }
                $result[$name] = $value;
            }
            else if (isset($pkeys[$name]))
            {
                $result[$name] = $value;
            }
        }

        if (!empty($exceptions))
        {
            throw new ModelFilterException($exceptions);
        }

        return $result;
    }

}



class ModelFilterException extends \Exception
{

    protected $exceptions;



    function __construct($exceptions, $message = null, $code = 0, $previous = null)
    {
        parent::__construct($message, $code, $previous);
        $this->exceptions = $exceptions;
    }



    function getExceptions()
    {
        return $this->exceptions;
    }

}
