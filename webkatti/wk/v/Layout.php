<?php /* @var $this \wk\c\Layout */ ?>

<header>
    <div class="wrapper">
        <h1>
            <a href="<?= wk\c\Link::prefix() ?>">WebKatti</a> <em>OpenArena statistics</em>
        </h1>

        <nav>
            <a href="<?= wk\c\Link::prefix() ?>">Home</a>
            <a href="<?= wk\c\Link::prefix('/games') ?>">Games</a>
            <a href="<?= wk\c\Link::prefix('/changes') ?>">Changes</a>
            <a href="<?= wk\c\Link::prefix('/about') ?>">About</a>
            <?php if (M::supervisor()->isAuthed()): ?>
            <a href="<?= wk\c\Link::prefix('/settings') ?>">Settings</a>
            <a href="<?= wk\c\Link::prefix('/clearCache') ?>">Clear cache</a>
            <a href="<?= wk\c\Link::prefix('/logout') ?>">Logout</a>
            <?php else: ?>
            <a href="<?= wk\c\Link::prefix('/login') ?>">Login</a>
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
