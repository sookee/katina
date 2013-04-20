<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;

use afw\Utils;
use afw\FileParams;



class Field
{

	/**
	 * @var \afw\c\Form\Field
	 */
	protected $formField;
	protected $name;
	protected $filters = [];
	/**
	 * @var FileParams
	 */
	protected $fileParams = null;



	function __construct($name)
	{
		$this->name = $name;
	}



	function getName()
	{
		return $this->name;
	}



	/**
	 * @return \afw\c\Form\Field
	 */
	function setFormField(\afw\c\Form\Field $field)
	{
		$this->formField = $field;
		$this->formField->name = $this->name;
		return $this;
	}



	/**
	 * @return \afw\c\Form\Field
	 */
	function getFormField()
	{
		return $this->formField;
	}



	/**
	 * @return Field
	 */
	function addFilter($filter)
	{
		$this->filters []= $filter;
		return $this;
	}



	function getFilters()
	{
		return $this->filters;
	}



	/**
	 * @return FileParams
	 */
	function dest($dest)
	{
		$this->fileParams =  new FileParams();
		return $this->fileParams->dest($dest);
	}



	function fileParams()
	{
		return $this->fileParams;
	}



	/**
	 * FieldFilters
	 */



	/**
	 * @return Field
	 */
	function sprintf($format)
	{

		return $this->addFilter(function($value) use ($format)
		{
			return sprintf($format, $value);
		});
	}



	/**
	 * @return Field
	 */
	function datef($format)
	{
		return $this->addFilter(function($value) use ($format)
		{
			$time = strtotime($value);
			return $time
				? date($format, strtotime($value))
				: null;
		});
	}



	/**
	 * @return Field
	 */
	function strip_tags($allowable_tags = null)
	{
		return $this->addFilter(function($value) use ($allowable_tags)
		{
			return strip_tags($value, $allowable_tags);
		});
	}



	public static $simple_tags_allowable = '<a><p><strong><em><ul><li><br><br/><br />';
	/**
	 * @return Field
	 */
	function simple_tags()
	{
		return $this->strip_tags(self::$simple_tags_allowable);
	}



	/**
	 * @return Field
	 */
	function nullint()
	{
		return $this->addFilter(function($value)
		{
			return !isset($value) || $value === '' ? null : (int)$value;
		});
	}



	/**
	 * @return Field
	 */
	function emptynull()
	{
		return $this->addFilter(function($value)
		{
			return empty($value) ? null : $value;
		});
	}



	/**
	 * @return Field
	 */
	function url()
	{
		return $this->addFilter(function($value)
		{
			return 'http://'.preg_replace('`^http://`', '', $value);
		});
	}



	/**
	 * @return Field
	 */
	function autoreplace($decodeEntities = false)
	{
		return $this->addFilter(function($value) use ($decodeEntities)
		{
			return Utils::autoreplace($value, $decodeEntities);
		});
	}



	/**
	 * @return Field
	 */
	function xhtml()
	{
		return $this->addFilter(function($value)
		{
			return Utils::xhtml($value);
		});
	}



	/**
	 * @return Field
	 */
	function simple_xhtml()
	{
		$this->simple_tags();
		return $this->xhtml();
	}



    function touri($uriFieldName = 'uri')
    {
        return $this->addFilter(function($value, $name, &$values, &$result) use ($uriFieldName)
        {
            $result[$uriFieldName] = Utils::str2uri($value);
            return $value;
        });
    }



	/**
	 * @return Field
	 */
	function hash($algo, $raw = false)
	{
		return $this->addFilter(function($value) use ($algo, $raw)
		{
			return hash($algo, $value, $raw);
		});
	}



	/**
	 * @return Field
	 */
	function bool()
	{
		return $this->addFilter(function($value)
		{
			return !empty($value);
		});
	}



	public static $error_required = 'Value required';



	/**
	 * @throws \Exception
	 * @return Field
	 */
	function required($error = null)
	{
		if (!isset($error))
		{
			$error = self::$error_required;
		}

		return $this->addFilter(function($value) use ($error)
		{
            if (is_scalar($value))
            {
                $value = trim($value);
            }
			if (empty($value) && (!is_numeric($value) || $value != 0))
			{
				throw new \Exception($error);
			}
			return $value;
		});
	}



	public static $error_match = 'Wrong value';



	/**
	 * @throws \Exception
	 * @return Field
	 */
	function match($pattern, $error = null)
	{
		if (!isset($error))
		{
			$error = self::$error_match;
		}

		return $this->addFilter(function($value) use ($pattern, $error)
		{
			if (!preg_match($pattern, $value))
			{
				throw new \Exception($error);
			}
			return $value;
		});
	}



	/**
	 * @return Field
	 */
	function email($error = null)
	{
		return $this->match('/^([\w\d\-\_\.]+\@[\w\d\-\_\.]+\.\w{2,4})?$/i', $error);
	}



	/**
	 * @return Field
	 */
	function phone($error = null)
	{
		return $this->match('/^[\d\s\+\-\,\(\)]*$/', $error);
	}



	public static $error_file = 'Wrong file type';



	/**
	 * @throws \Exception
	 * @return Field
	 */
	function file($nameFilter = '*.*', $error = null)
	{
		if (!isset($error))
		{
			$error = self::$error_file;
		}

		return $this->addFilter(function($value, $name, $values) use ($nameFilter, $error)
		{
            if (!empty($values[$name . '-delete']))
            {
                return '';
            }
            if (empty($value['tmp_name']) || !file_exists($value['tmp_name']))
            {
                throw new ModelFilterSkip();
            }
            if (!preg_match('/^'.str_replace(['.', '*', ';'], ['\.', '.*', '|'], $nameFilter).'$/i', $value['name']))
            {
                throw new \Exception($error);
            }
			return substr($value['name'], strrpos($value['name'], '.') + 1);
		});
	}



	/**
	 * @return Field
	 */
	function image($error = null, $nameFilter = '*.jpg;*.gif;*.png')
	{
		return $this->file($nameFilter, $error);
	}



}
