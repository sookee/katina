<?php

/*
 * Copyright © 2013 Krylosov Maksim <Aequiternus@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace afw;



class AMarkup
{

    public $linkMaxLength;
    public $headingDec;
    public $allowable;
    public $linkNoFollow;

    private $stack = [];
    private $current = 'p';
    private $line;



    function __construct($linkMaxLength = 50, $headingDec = 0, $allowable = '=*#!-&|`', $linkNoFollow = true)
    {
        $this->linkMaxLength = $linkMaxLength;
        $this->headingDec = $headingDec;
        $this->allowable = $allowable;
        $this->linkNoFollow = $linkNoFollow;
    }



    function convert($text)
    {
        $text = trim($text);
        if (empty($text))
        {
            return $text;
        }
        $text = htmlspecialchars($text);
        $text = $this->lineBreak($text);
        $lines = explode("\n", $text);
        $text = '';

        foreach ($lines as $line)
        {
            $this->line = $line;
            if ($this->current == "`")
            {
                $this->precode();
            }
            else if (empty($this->line))
            {
                $this->paragraph();
            }
            else
            {
                if (strpos($this->allowable, $this->line[0]) !== false)
                {
                    switch ($this->line[0])
                    {
                        case '=': $this->heading(); break;
                        case '*': $this->unordered(); break;
                        case '#': $this->ordered(); break;
                        case '!': $this->term(); break;
                        case '-': $this->descr(); break;
                        case '&': $this->blockquote(); break;
                        case '|': $this->table(); break;
                        case '`': $this->precode(); break;
                        default : $this->plaintext(); break;
                    }
                }
                else
                {
                    $this->plaintext();
                }
                $this->replace();
                $this->link();
            }
            $text .= $this->line;
        }

        return trim($text . $this->flush());
    }



    private function lineBreak($text)
    {
        $text = str_replace(["\r\n", "\r"], "\n", $text);
        $text = preg_replace('`\n{3,}`', "\n\n", $text);
        return $text;
    }



    private function replace()
    {
        $this->line = preg_replace([
            '`([^\s(-])&quot;`',
            '`&quot;([^\s)])`',
            '`(\s|&nbsp;)--?-?(\s)`',
            '`---`',
            '`--`',
            '`&lt;&lt;`',
            '`&gt;&gt;`',
            '`\(c\)`i',
            '/`([^`]+?)`/',
            '`\[(\#|/|\./)(\w+)\s+([^]]+)\]`',
            '`(\S+)\(([^\)]+)\)`',
        ], [
            '$1»',
            '«$1',
            ' —$2',
            '—',
            '–',
            '«',
            '»',
            '©',
            '<code>$1</code>',
            '<a href="$1$2">$3</a>',
            '<abbr title="$2">$1</abbr>',
        ], $this->line);
    }



    private function link()
    {
        $this->line = preg_replace_callback(
            '`(\[|\b)(([\w-]+://?|www[.])[^\s()<>]+(?:\([\w\d]+\)|([^[:punct:]\s]|/)))(\s+([^\[\]]+)\])?`',
            function($matches)
            {
                $url = $matches[2];
                if (!empty($matches[1]) && !empty($matches[6]))
                {
                    $text = $matches[6];
                }
                else
                {
                    $text = htmlspecialchars_decode($url);
                    $text = strlen($text) > $this->linkMaxLength
                        ? substr($text, 0, $this->linkMaxLength) . '…'
                        : $text;
                    $text = htmlspecialchars($text);
                }
                return '<a'
                    . ($this->linkNoFollow ? ' rel="nofollow"' : '')
                    . ' href="'
                    . ($matches[3] == 'www.' ? 'http://' : '')
                    . $url
                    . '">' . $text . '</a>';
            },
            $this->line
        );
    }



    private function paragraph()
    {
        $this->line = $this->flush();
        $this->current = 'p';
    }



    private function plaintext()
    {
        if ($this->current == 'p')
        {
            $this->line = ' <p> ' . $this->line;
            $this->current = null;
        }
        $this->line .= "\n";
    }



    private function heading()
    {
        if (!preg_match('`^(\=+)(\#(\w+))?\s(.*)$`', $this->line, $matches))
        {
            return;
        }

        $level = min(6, strlen(@$matches[1]) + $this->headingDec);
        $name = @$matches[3];

        $this->line = $this->flush()
            . ' <h' . $level
            . (empty($name) ? '' : ' id="' . htmlspecialchars($name) . '"')
            . '> ' . @$matches[4] . ' </h' . $level . '> ';
    }



    private function _list($type1, $type2)
    {
        if (!preg_match("`^([{$type1}]+)\s(.*)$`", $this->line, $matches))
        {
            $this->plaintext();
            return;
        }

        $level = strlen($matches[1]);
        $this->line = @$matches[2];

        if ($this->current[0] == $type1)
        {
            $currLevel = strlen($this->current);
            if ($currLevel == $level)
            {
                $this->line = ' <li> ' . $this->line;
            }
            else if ($currLevel < $level)
            {
                $this->line = " <{$type2}l><li> " . $this->line;
                $this->stack []= " </{$type2}l> ";
            }
            else
            {
                $this->line = array_pop($this->stack) . ' <li> ' . $this->line;
            }
        }
        else
        {
            $this->line = " <{$type2}l><li> " . $this->line;
            $this->stack []= " </{$type2}l> ";
        }
        $this->line .= "\n";
        $this->current = $matches[1];
    }



    private function unordered()
    {
        $this->_list('*', 'u');
    }



    private function ordered()
    {
        $this->_list('#', 'o');

    }



    private function term()
    {
        if ($this->line[1] != ' ')
        {
            $this->plaintext();
            return;
        }
        $this->line = substr($this->line, 2);
        if ($this->current != '!')
        {
            $this->line = $this->flush() . ' <dl><dt> ' . $this->line;
            $this->stack []= ' </dl> ';
            $this->current = '!';
        }
        else
        {
            $this->line = ' <dt> ' . $this->line;
        }
        $this->line .= "\n";
    }



    private function descr()
    {
        if ($this->line[1] != ' ')
        {
            $this->plaintext();
            return;
        }
        if ($this->current == '!')
        {
            $this->line = ' <dd> ' . substr($this->line, 2);
        }
        $this->line .= "\n";
    }



    private function blockquote()
    {
        if (substr($this->line, 0, 4) != '&gt;')
        {
            $this->plaintext();
            return;
        }
        $this->line = substr($this->line, 4);
        if ($this->current != '>')
        {
            $this->line = $this->flush() . ' <blockquote> ' . $this->line . "\n";
            $this->stack []= ' </blockquote> ';
            $this->current = '>';
        }
        else
        {
            $this->line .= "\n";
        }
    }



    private function table()
    {
        if (substr($this->line, 0, 3) == '|= ')
        {
            $tag = 'th';
        }
        else if ($this->line[1] == ' ')
        {
            $tag = 'td';
        }
        else
        {
            $this->plaintext();
            return;
        }
        $buf = substr($this->line, 2);
        if ($this->current != '|')
        {
            $this->line = $this->flush() . ' <table>';
            $this->stack []= '</table> ';
            $this->current = '|';
        }
        else
        {
            $this->line = '';
        }

        $this->line .= "<tr><$tag>" . preg_replace('`\s{2,}`', "</$tag><$tag>", trim($buf)) . "</$tag></tr>\n";
    }



    private function precode()
    {
        if ($this->line == '```')
        {
            if ($this->current != '`')
            {
                $this->current = '`';
                $this->line = ' <pre><code>';
            }
            else
            {
                $this->current = null;
                $this->line = '</code></pre> ';
            }
        }
        else
        {
            $this->plaintext();
        }
    }



    private function flush()
    {
        $result = '';
        for ($i = count($this->stack) - 1; $i >= 0; $i--)
        {
            $result .= $this->stack[$i];
        }
        $this->stack = [];
        $this->current = 'p';
        return $result;
    }

}
