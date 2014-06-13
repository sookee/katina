<?php

namespace afw;



class Http
{

    static function request($url, $data = [])
    {
        if (empty($data))
        {
            $context = null;
        }
        else
        {
            $context = stream_context_create([
                'http' => [
                    'method' => 'POST',
                    'headers' => 'Content-type: application/x-www-form-urlencoded',
                    'content' => http_build_query($data),
                ]
            ]);
        }
        return file_get_contents($url, null, $context);
    }



    static function json_decode($url, $data = [])
    {
        return json_decode(self::request($url, $data));
    }



    static function unserialize($url, $data = [])
    {
        return unserialize(self::request($url, $data));
    }



    static function simplexml($url, $data = [])
    {
        return simplexml_load_string(self::request($url, $data));
    }

}
