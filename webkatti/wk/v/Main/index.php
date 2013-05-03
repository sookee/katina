<h2>Statistics <?= afw\Utils::datef('d.m.Y', $this->dateFrom) ?> &mdash; <?= afw\Utils::datef('d.m.Y', $this->dateTo) ?></h2>
<form action="" method="get">
    <?php if (!empty($this->error)): ?>
        <p class="error"><?= $this->error ?></p>
    <?php endif ?>
    <p>
        <label class="games-date">
            Date from
            <input class="games-date" type="date" size="8" name="from" value="<?= $this->dateFrom ?>">
        </label>
        &emsp;
        <label class="games-date">
            Date to
            <input class="games-date" type="date" size="8" name="to" value="<?= $this->dateTo ?>">
        </label>
        &emsp;
        <input class="games-date-submit" type="submit" value="   OK   ">
    </label>
</form>
<?php $this->players->render() ?>
