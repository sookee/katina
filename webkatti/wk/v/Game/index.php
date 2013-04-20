<h2>Games <?= afw\Utils::datef('d.m.Y', $this->date) ?></h2>
<form action="" method="get">
    <?php if (!empty($this->error)): ?>
        <p class="error"><?= $this->error ?></p>
    <?php endif ?>
    <label>
        Enter date
        <input class="games-date" type="date" size="8" name="date" value="<?= $this->date ?>">
        <input class="games-date-submit" type="submit" value="   OK   ">
    </label>
</form>
<?php $this->games->render() ?>
