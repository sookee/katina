<?php /* @var $this \wk\c\Layout */ ?>

<header>
    <div class="wrapper">
        <h1>
            <a href="/">WebKatti</a> <em>OpenArena statistics</em>
        </h1>

        <nav>
            <a href="/">Home</a>
            <a href="/games">Games</a>
            <a href="/changes">Changes</a>
            <a href="/about">About</a>
            <?php if (M::supervisor()->isAuthed()): ?>
            <!--<a href="/settings">Settings</a>-->
            <a href="/clearCache">Clear cache</a>
            <a href="/logout">Logout</a>
            <?php else: ?>
            <a href="/login">Login</a>
            <?php endif ?>
        </nav>
    </div>
</header>

<div class="wrapper">
    <?php $this->contents(); ?>

    <footer>
        <?php
            global $microtime;
            printf(
                'Time: %.3f с&emsp;Memory: %.3f Мб&emsp;Cache: %.3f Мб&emsp;Sql: %d / %d&emsp;',
                microtime(true) - $microtime,
                memory_get_peak_usage() / pow(2, 20),
                Storage::cache()->size() / pow(2, 20),
                Storage::main()->queryCount(),
                Storage::main()->queryCount() + Storage::main()->cachedCount()
            );
        ?>
    </footer>
</div>
