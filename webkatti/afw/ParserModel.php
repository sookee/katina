<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



abstract class ParserModel extends ParserList
{

    /**
     * @var m\Model
     */
    public $model;
    public $ekey = 'eid';
    public $skey = 'sid';
    public $oldKey = 'old';
    public $updateFiles = true;
    public $exceptions = [];
    public $countParsed = 0;
    public $countInsert = 0;
    public $countUpdate = 0;
    protected $files = [];



    function __construct(m\Model $model)
    {
        $this->model = $model;
    }



    function reset()
    {
        parent::reset();
        $this->exceptions = [];
        $this->countParsed = 0;
        $this->countInsert = 0;
        $this->countUpdate = 0;
    }



    function flushOldKey()
    {
        $this->model->db()
            ->where($this->ekey . ' IS NOT NULL')
            ->update([$this->oldKey => true]);
    }



    function deleteOld()
    {
        $this->model->db()
            ->key(true, $this->oldKey)
            ->where($this->ekey . ' IS NOT NULL')
            ->delete();
        return $this->model->db()->lastRowCount();
    }



    protected function parseItem()
    {
        try
        {
            if (isset($this->oldKey))
            {
                $this->item[$this->oldKey] = false;
            }

            $old = $this->model->db()
                ->key($this->item[$this->ekey], $this->ekey)
                ->key($this->item[$this->skey], $this->skey)
                ->first();

            if (!empty($old))
            {
                if ($this->updateFiles)
                {
                    $this->loadFiles();
                }
                $this->model->update($this->mergeItem($old), $old[$this->model->pkey]);
                $this->countUpdate++;
            }
            else
            {
                $this->loadFiles();
                $this->model->insert($this->item);
                $this->countInsert++;
            }

            $this->clearFiles();
            $this->countParsed++;
        }
        catch (\afw\m\ModelFilterException $e)
        {
            foreach ($e->getExceptions() as $exception);
            {
                $this->exceptions[$this->url][] = $exception;
            }
        }
        catch (\Exception $e)
        {
            $this->exceptions[$this->url][] = $exception;
        }
    }



    protected function mergeItem($old)
    {
        return $this->item + $old;
    }



    protected function loadFiles()
    {
        $this->files = [];
        foreach ($this->model->fields as $name => $field)
        {
            $params = $field->fileParams();
            if (!empty($params))
            {
                $this->preload();
                $this->files []= $file = tempnam(null, 'parse');
                file_put_contents($file, file_get_contents($this->item[$name]));
                $this->item[$name] = [
                    'name' => basename($this->item[$name]),
                    'tmp_name' => $file
                ];
            }
        }
    }



    protected function clearFiles()
    {
        foreach ($this->files as $file)
        {
            @unlink($file);
        }
    }

}
