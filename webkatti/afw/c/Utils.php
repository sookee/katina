<?php

/*
 * Copyright © 2013 Aequiternus@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw\c;



class Utils
{

    static function redrawImages(\afw\m\Model $model, $fieldImage)
    {
        $params = $model->getField($fieldImage)->fileParams()->export();

		$max_i = null;
		$max_w = 0;
		foreach ($params as $i=>$p)
		{
			if (empty($p[\afw\File::IMAGE_WIDTH]) || empty($p[\afw\File::IMAGE_HEIGHT]))
			{
				$max_i = $i;
				break;
			}

			if ($p[\afw\File::IMAGE_WIDTH] > $max_w)
			{
				$max_w = $p[\afw\File::IMAGE_WIDTH];
				$max_i = $i;
			}
		}
        unset($params[$max_i]);

        $r = $model->db()
            ->fields([$model->pkey, $fieldImage])
            ->where("$fieldImage != ''");

        foreach ($r as $row)
        {
            \afw\File::receive(
                $model->dest($fieldImage, $row, $max_i),
                $params,
                $model->destArgs($fieldImage, $row)
            );
        }
    }

}
