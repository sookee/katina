<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\m;



class Settings extends ModelWithFields
{

    protected $file;
    protected $class;
    protected $data;



    function __construct($file, $class = 'Config')
    {
        $this->file = $file;
        $this->class = $class;
    }



    protected function load()
    {
        if (!isset($this->data))
        {
            try
            {
                $reflection = new \ReflectionClass($this->class);
                $this->data = $reflection->getStaticProperties();
            }
            catch (\ReflectionException $e)
            {
                $this->data = [];
            }
        }
    }



    function save(array $rawValues)
    {
        $this->load();
        $values = $this->filterValues($rawValues) + $this->data;

        $s = "<?php\n\nclass {$this->class}\n{\n\n";
        foreach ($this->fields as $name => $field)
        {
            $formField = $field->getFormField();
            if (isset($formField))
            {
                if (!empty($formField->label))
                {
                    $s .= "    # {$formField->label}\n";
                }
                if (!empty($formField->description))
                {
                    $s .= "    # {$formField->description}\n";
                }
            }
            $s .= "    static \$$name = "
                . var_export(@$values[$name], true)
                . ";\n\n";
        }
        $s .= "}\n";

        file_put_contents($this->file, $s, LOCK_EX);
    }



    function get($name)
    {
        $this->load();
        return @$this->data[$name];
    }



    function all()
    {
        $this->load();
        return $this->data;
    }

}
