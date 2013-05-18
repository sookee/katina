<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class File
{

    const DEST = 'dest';
    const IMAGICK = 'imagick';
    const IMAGE_WIDTH = 'width';
    const IMAGE_HEIGHT = 'height';
    const IMAGE_BACKGROUND = 'background';
    const IMAGE_QUALITY = 'quality';
    const IMAGE_REDUCTION = 'reduction';
    const REDUCTION_PROPORTIONAL = 'proportional';
    const REDUCTION_UNPROPORTIONAL = 'unproportional';
    const REDUCTION_BEGIN = 'begin';
    const REDUCTION_CENTER = 'center';
    const REDUCTION_END = 'end';
    const WATERMARK = 'watermark';
    const WATERMARK_WIDTH = 'wm_width';
    const WATERMARK_HEIGHT = 'wm_height';
    const WATERMARK_LEFT = 'wm_left';
    const WATERMARK_TOP = 'wm_top';
    const IMAGE_FORMAT = 'format';
    const FORMAT_JPEG = 'jpg';
    const FORMAT_GIF = 'gif';
    const FORMAT_PNG = 'png';
    const FORMAT_ORIGINAL = 'original';

    public static $SIZE_ERR = 'Слишком большой размер изображения (%dx%d)';
    public static $IN_FORMAT_ERR = 'Формат файла не поддерживается';
    public static $OUT_FORMAT_ERR = 'Формат выходного файла не поддерживается';
    public static $WRITE_ERR = 'Не удалось записать файл';
    public static $IMAGICK_ERR = 'Ошибка обработки изображения: %s';
    public static $CHMOD = 0664;
    public static $DIR_CHMOD = 0775;
    public static $JPEG_QUALITY = 90;
    public static $PNG_QUALITY = 9;
    public static $UNLINK_RECEIVE_SRC = false;



    static function checkDir($filename, $mode = null)
    {
        $pathname = dirname($filename);
        if (!file_exists($pathname))
        {
            mkdir($pathname, $mode ? : self::$DIR_CHMOD, true);
        }
    }



    private static function prepareParams(&$params, $args = null, $index = null)
    {
        if (empty($params))
        {
            return;
        }
        if ($params instanceof FileParams)
        {
            $params = $params->export();
        }
        if (!isset($params[0]))
        {
            $params = [$params];
        }
        if (isset($index))
        {
            $params[$index][self::DEST] = vsprintf($params[$index][self::DEST], (array)$args);
        }
        else
        {
            foreach ($params as &$p)
            {
                if (isset($p[self::DEST]))
                {
                    $p[self::DEST] = vsprintf($p[self::DEST], (array)$args);
                }
            }
        }
    }



    static function delete($params, $args = null)
    {
        if (empty($params))
        {
            return;
        }
        self::prepareParams($params, $args);

        foreach ($params as $p)
        {
            if (file_exists($p[self::DEST]))
            {
                unlink($p[self::DEST]);
            }
        }
    }



    static function receive($src, $params, $args = null, $mode = null)
    {
        if (empty($params))
        {
            return null;
        }
        self::prepareParams($params, $args);
        if (isset($params[0][self::IMAGICK]))
        {
            self::receiveImageMagick($src, $params, $mode);
        }
        else if (count($params[0]) > 1)
        {
            self::receiveImageGD($src, $params, $mode);
        }
        else
        {
            self::receiveFile($src, $params, $mode);
        }
    }



    static function dest($params, $index = 0, $args = null)
    {
        if (empty($params))
        {
            return null;
        }
        self::prepareParams($params, $args, $index);
        return $params[$index][self::DEST];
    }



    static function move($src_params, $dest_params, $src_args = null, $dest_args = null)
    {
        if (empty($src_params) || empty($dest_params))
        {
            return;
        }
        self::prepareParams($src_params, $src_args);
        self::prepareParams($dest_params, $dest_args);

        foreach ($src_params as $i => $v)
        {
            if (empty($dest_params[$i]))
            {
                continue;
            }

            $src = $v[self::DEST];
            $dest = $dest_params[$i][self::DEST];

            if (file_exists($src))
            {
                self::checkDir($dest);
                unlink($dest);

                rename($src, $dest);
            }
        }
    }



    static function copy($src_params, $dest_params, $src_args = null, $dest_args = null)
    {
        if (empty($src_params) || empty($dest_params))
        {
            return;
        }
        self::prepareParams($src_params, $src_args);
        self::prepareParams($dest_params, $dest_args);

        foreach ($src_params as $i => $v)
        {
            if (empty($dest_params[$i]))
            {
                continue;
            }

            $src = $v[self::DEST];
            $dest = $dest_params[$i][self::DEST];

            if (file_exists($src))
            {
                self::checkDir($dest);
                unlink($dest);

                if (!copy($src, $dest))
                {
                    throw new \Exception(self::$WRITE_ERR);
                }
            }
        }
    }



    private static function receiveFile($src, $params, $mode = null)
    {
        foreach ($params as $p)
        {
            self::checkDir($p[self::DEST]);
            if (!rename($src, $p[self::DEST]))
            {
                throw new \Exception(self::$WRITE_ERR);
            }
            chmod($p[self::DEST], $mode ? : self::$CHMOD);
        }
        return 0;
    }



    private static function checkPercent($src, $value)
    {
        if (substr($value, -1) == '%')
        {
            return $src * 0.01 * (int)$value;
        }
        else return $value;
    }



    private static function receiveImageGD($src, array $params, $mode = null)
    {
        list($w, $h, $type/* , $a */) = getimagesize($src);
        $memory_limit = (int)ini_get('memory_limit');
        if ($memory_limit > 0 && $w * $h * 5 / 1048576 > $memory_limit)
        {
            throw new \Exception(sprintf(self::$SIZE_ERR, $w, $h));
        }

        switch ($type)
        {
            case IMAGETYPE_JPEG:
                $img = imagecreatefromjpeg($src);
                break;
            case IMAGETYPE_GIF:
                $img = imagecreatefromgif($src);
                break;
            case IMAGETYPE_PNG:
                $img = imagecreatefrompng($src);
                break;
            default:
                throw new \Exception(self::$IN_FORMAT_ERR);
        }

        foreach ($params as $p)
        {
            $w2 = $w;
            $h2 = $h;
            $l = 0;
            $t = 0;
            $rw = $w;
            $rh = $h;

            if (!empty($p[self::IMAGE_WIDTH]) && !empty($p[self::IMAGE_HEIGHT]))
            {
                if (empty($p[self::IMAGE_REDUCTION]) || $p[self::IMAGE_REDUCTION] == self::REDUCTION_PROPORTIONAL)
                {
                    if ($w2 > $p[self::IMAGE_WIDTH])
                    {
                        $w2 = $p[self::IMAGE_WIDTH];
                        $h2 = round($h / $w * $w2);
                    }
                    if ($h2 > $p[self::IMAGE_HEIGHT])
                    {
                        $h2 = $p[self::IMAGE_HEIGHT];
                        $w2 = round($w / $h * $h2);
                    }
                }
                else if ($p[self::IMAGE_REDUCTION] == self::REDUCTION_UNPROPORTIONAL)
                {
                    $w2 = $p[self::IMAGE_WIDTH];
                    $h2 = $p[self::IMAGE_HEIGHT];
                }
                else
                {
                    $w2 = $p[self::IMAGE_WIDTH];
                    $h2 = $p[self::IMAGE_HEIGHT];
                    if ($w / $h > $w2 / $h2)
                    {
                        $rw = round($w2 / $h2 * $h);
                        switch ($p[self::IMAGE_REDUCTION])
                        {
                            case self::REDUCTION_CENTER:
                                $l = round($w / 2 - $rw / 2);
                                break;
                            case self::REDUCTION_END:
                                $l = $w - $rw;
                                break;
                        }
                    }
                    else if ($w / $h < $w2 / $h2)
                    {
                        $rh = round($h2 / $w2 * $w);
                        switch ($p[self::IMAGE_REDUCTION])
                        {
                            case self::REDUCTION_CENTER:
                                $t = round($h / 2 - $rh / 2);
                                break;
                            case self::REDUCTION_END:
                                $t = $h - $rh;
                                break;
                        }
                    }
                }
            }
            else if (!empty($p[self::IMAGE_WIDTH]))
            {
                if ($w2 > $p[self::IMAGE_WIDTH])
                {
                    $w2 = $p[self::IMAGE_WIDTH];
                    $h2 = round($h / $w * $w2);
                }
            }
            else if (!empty($p[self::IMAGE_HEIGHT]))
            {
                if ($h2 > $p[self::IMAGE_HEIGHT])
                {
                    $h2 = $p[self::IMAGE_HEIGHT];
                    $w2 = round($w / $h * $h2);
                }
            }

            $img2 = imagecreatetruecolor($w2, $h2);

            if (!empty($p[self::IMAGE_BACKGROUND]))
            {
                $bg = imagecolorallocate(
                    $img2,
                    hexdec(substr($p[self::IMAGE_BACKGROUND], 0, 2)),
                    hexdec(substr($p[self::IMAGE_BACKGROUND], 2, 2)),
                    hexdec(substr($p[self::IMAGE_BACKGROUND], 4, 2))
                );
            }
            else
            {
                imagesavealpha($img2, true);
                $bg = imagecolorallocatealpha($img2, 0, 0, 0, 127);
            }
            imagefill($img2, 0, 0, $bg);

            imagecopyresampled($img2, $img, 0, 0, $l, $t, $w2, $h2, $rw, $rh);

            if (isset($p[self::WATERMARK]))
            {
                $img3 = imagecreatefrompng($p[self::WATERMARK]);
                $w3 = imagesx($img3);
                $h3 = imagesy($img3);
                if (!empty($p[self::WATERMARK_WIDTH]) && !empty($p[self::WATERMARK_HEIGHT]))
                {
                    $w4 = self::checkPercent($w2, $p[self::WATERMARK_WIDTH]);
                    $h4 = self::checkPercent($h2, $p[self::WATERMARK_HEIGHT]);
                }
                else if (!empty($p[self::WATERMARK_WIDTH]))
                {
                    $w4 = self::checkPercent($w2, $p[self::WATERMARK_WIDTH]);
                    $h4 = ceil($h3 / $w3 * $w4);
                }
                else if (!empty($p[self::WATERMARK_HEIGHT]))
                {
                    $h4 = self::checkPercent($h2, $p[self::WATERMARK_HEIGHT]);
                    $w4 = ceil($w3 / $h3 * $h4);
                }
                else
                {
                    $w4 = $w3;
                    $h4 = $h3;
                }

                $wl = self::checkPercent($w2, $p[self::WATERMARK_LEFT]);
                $wt = self::checkPercent($h2, $p[self::WATERMARK_TOP]);

                if ($wl < 0) $wl = $w2 + $wl - $w4;
                if ($wt < 0) $wt = $h2 + $wt - $h4;

                imagecopyresampled($img2, $img3, $wl, $wt, 0, 0, $w4, $h4, $w3, $h3);
                imagedestroy($img3);
            }

            self::checkDir($p[self::DEST]);

            if (!isset($p[self::IMAGE_FORMAT]))
            {
                $p[self::IMAGE_FORMAT] = substr($p[self::DEST], strrpos($p[self::DEST], '.') + 1);
            }
            else if ($p[self::IMAGE_FORMAT] == self::FORMAT_ORIGINAL)
            {
                $p[self::IMAGE_FORMAT] = $type;
            }

            switch ($p[self::IMAGE_FORMAT])
            {
                case 'jpeg': case 'JPEG': case 'jpg': case 'JPG': case IMAGETYPE_JPEG:
                    if (!isset($p[self::IMAGE_QUALITY]))
                    {
                        $p[self::IMAGE_QUALITY] = self::$JPEG_QUALITY;
                    }
                    $r = imagejpeg($img2, $p[self::DEST], $p[self::IMAGE_QUALITY]);
                    break;
                case 'gif': case 'GIF': case IMAGETYPE_GIF:
                    $r = imagegif($img2, $p[self::DEST]);
                    break;
                case 'png': case 'PNG': case IMAGETYPE_PNG:
                    if (!isset($p[self::IMAGE_QUALITY]))
                    {
                        $p[self::IMAGE_QUALITY] = self::$PNG_QUALITY;
                    }
                    $r = imagepng($img2, $p[self::DEST], $p[self::IMAGE_QUALITY]);
                    break;
                default:
                    imagedestroy($img2);
                    imagedestroy($img);
                    throw new \Exception(self::$OUT_FORMAT_ERR);
            }
            imagedestroy($img2);

            if ($r)
            {
                chmod($p[self::DEST], $mode ? : self::$CHMOD);
                if (self::$UNLINK_RECEIVE_SRC)
                {
                    unlink($src);
                }
            }
            else
            {
                imagedestroy($img);
                throw new \Exception(self::$WRITE_ERR);
            }
        }

        imagedestroy($img);
        return 0;
    }



    private static function receiveImageMagick($src, array $params, $mode = null)
    {
        foreach ($params as $p)
        {
            self::checkDir($p[self::DEST]);
            $r = trim(shell_exec(escapeshellcmd("convert {$p[self::IMAGICK]} $src {$p[self::DEST]}") . ' 2>&1'));
            if (!empty($r))
            {
                throw new \Exception(sprintf(self::$IMAGICK_ERR, $r));
            }
            chmod($p[self::DEST], $mode ? : self::$CHMOD);
            if (self::$UNLINK_RECEIVE_SRC)
            {
                unlink($src);
            }
        }
    }

}
