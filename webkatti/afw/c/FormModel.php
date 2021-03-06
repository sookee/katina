<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;



class FormModel extends FormModelWithFields
{

	public $id;

    public static $format_complete_created = '%s created';
    public static $format_complete_updated = '%s saved';
    public static $format_complete_deleted = '%s deleted';
    public static $format_title_create = 'Create %s';
    public static $format_title_update = 'Save %s';
    public static $format_title_delete = 'Delete %s';
    public static $format_button_create = 'Create';
    public static $format_button_update = 'Save';
    public static $format_button_delete = 'Delete';



	function __construct($completeMessage, \afw\m\Model $model, $id = null)
	{
		parent::__construct($completeMessage, $model);

		$this->id = $id;
	}



	function push(Controller $controller)
	{
		if ($controller instanceof Form\FieldFile && !isset($controller->src) && isset($this->id))
        {
            $controller->src = $this->model->dest($controller->name, $this->data + $this->default, 0, $this->id);
		}
		parent::push($controller);
	}



	/**
	 * @return FormModel
	 */
	static function insert(\afw\m\Model $model,
        $completeMessage, $headLabel = null, $pushFields = null, $buttonLabel = null)
	{
		$c = new static($completeMessage, $model);
		$c->setData($_POST);
        $c->run(function() use ($c)
        {
            $c->id = $c->model->insert($_POST + $_FILES);
        });
        $c->pushElements($headLabel, $pushFields, $buttonLabel);
		return $c;
	}



	/**
	 * @return FormModel
	 */
	static function update(\afw\m\Model $model, $id,
        $completeMessage, $headLabel = null, $pushFields = null, $buttonLabel = null)
	{
        $oldValues = $model->get($id);
		$c = new static($completeMessage, $model, $id);
		$c->setData($_POST);
		$c->setDefault($oldValues);
        $c->run(function() use ($c, $id, $oldValues)
        {
            $c->model->update($_POST + $_FILES, $id, $oldValues, $newValues);
            $c->setDefault($newValues + $oldValues);
        });
        $c->pushElements($headLabel, $pushFields, $buttonLabel);
		return $c;
	}



	/**
	 * @return FormModel
	 */
	static function delete(\afw\m\Model $model, $id,
        $completeMessage, $headLabel = null, $pushFields = null, $buttonLabel = null)
	{
        $oldValues = $model->get($id);
		$c = new static($completeMessage, $model, $id);
		$c->setDefault($oldValues);
        $c->run(function() use ($c, $id, $oldValues)
        {
            $c->model->delete($id, $_POST, $oldValues);
        });
        $c->pushElements($headLabel, $pushFields, $buttonLabel);
		return $c;
	}



    /**
     * @return FormModel
     */
    static function variable($model, $method, $id, array $allowable)
    {
        if (
            (
                !@in_array($method, $allowable[$model])
                && !in_array($model, $allowable)
            )
            || $method == $allowable[$model][0]
            || !in_array($method, ['insert', 'update', 'delete'])
        ){
			throw new \afw\HttpException(404);
		}

        /* @var $model \afw\m\Model */
        $model = $allowable[$model][0];
		if (!($model instanceof \afw\m\Model))
		{
			throw new \afw\HttpException(404);
		}

        $title = $model->getTitle();
        switch ($method)
        {
            case 'insert':
                return self::insert(
                    $model,
                    sprintf(self::$format_complete_created, $title),
                    sprintf(self::$format_title_create, $title),
                    true,
                    sprintf(self::$format_button_create, $title)
                );
            case 'update':
                return self::update(
                    $model,
                    $id,
                    sprintf(self::$format_complete_updated, $title),
                    sprintf(self::$format_title_update, $title),
                    true,
                    sprintf(self::$format_button_update, $title)
                );
            case 'delete':
                return self::delete(
                    $model,
                    $id,
                    sprintf(self::$format_complete_deleted, $title),
                    sprintf(self::$format_title_delete, $title),
                    [$model->pkey => $model->pkey],
                    sprintf(self::$format_button_delete, $title)
                );
        }
    }


}
