<?php

/**
 * @link http://code.google.com/p/htmlcutting/
 */

namespace afw\ext;



class HTMLCutter
{

    private static $fakeSymb = "\r";
    private static $tags = array();
    private static $tagCounter = 0;
    private static $openTags = array();
    private static $closeTags = array();
    private static $exTags = array('br');



    private static function tagOut($tag)
    {
        self::$tagCounter++;
        self::$tags[self::$tagCounter] = $tag;
        return self::$fakeSymb;
    }



    private static function tagIn()
    {
        self::$tagCounter++;
        $tag = self::$tags[self::$tagCounter];

        preg_match('/^<(\/?)(\w+)[^>]*>/i', $tag, $matches);
        if (!in_array($matches[2], self::$exTags))
        {
            if ($matches[1] != '/')
            {
                self::$openTags[] = $matches[2];
            }
            else
            {
                self::$closeTags[] = $matches[2];
            }
        }
        return $tag;
    }



    static function cut($text, $length)
    {
        if (mb_strlen($text) <= $length)
        {
            return $text;
        }

        $text = html_entity_decode($text);
        $text = preg_replace('/' . self::$fakeSymb . '/', '', $text);

        //move all tags to array tags
        $text = preg_replace('/(<\/?)(\w+)([^>]*>)/e', '\afw\ext\HTMLCutter::tagOut("$0")', $text);

        //check how many tags in cutter text to fix cut length
        $preCut = mb_substr($text, 0, $length);
        $fakeCount = mb_substr_count($preCut, self::$fakeSymb);
        //cut string
        $text = mb_substr($text, 0, $length + ($fakeCount * mb_strlen(self::$fakeSymb)));
        //remove last word
        $text = preg_replace('/\S*$/', '', $text);

        //return tags back
        self::$tagCounter = 0;
        $text = preg_replace('/' . self::$fakeSymb . '/e', '\afw\ext\HTMLCutter::tagIn()', $text);

        //get count not closed tags
        $closeCount = count(self::$openTags) - count(self::$closeTags);
        //close opened tags
        for ($i = 0; $i < $closeCount; $i++)
        {
            $tagName = array_pop(self::$openTags);
            $text .= "</{$tagName}>";
        }

        return $text;
    }

}
