<?php

/* @var $this \afw\c\Paginator */

if ($this->count <= 1)
{
    return;
}

echo '<nav class="paginator">';

if ($this->current == 1)
{
    echo '<strong>1</strong>';
}
else
{
    echo ' <a href="', $this->href($this->current - 1), '">&larr;</a>';
    echo ' <a href="', $this->href(1), '">1</a>';
}

if ($this->current - $this->radius > 2)
{
    echo ' <span>...</span>';
}

for ($i = 2; $i < $this->count; $i++)
{
    if (abs($i - $this->current) <= $this->radius)
    {
        if ($this->current == $i)
        {
            echo ' <strong>', $i, '</strong>';
        }
        else
        {
            echo ' <a href="', $this->href($i), '">', $i, '</a>';
        }
    }
}

if ($this->current + $this->radius < $this->count - 1)
{
    echo ' <span>...</span>';
}

if ($this->current == $this->count)
{
    echo ' <strong>', $this->count, '</strong>';
}
else
{
    echo ' <a href="', $this->href($this->count), '">', $this->count, '</a>';
    echo ' <a href="', $this->href($this->current + 1), '">&rarr;</a>';
}

echo '</nav>';
