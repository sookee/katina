<h2>
    Statistics,
    <?php

        if ($this->type == 'date')
        {
            echo afw\Utils::datef('d.m.Y', $this->dateFrom), ' &mdash; ', afw\Utils::datef('d.m.Y', $this->dateTo);
        }
        else
        {
            echo date('F', mktime(0, 0, 0, $this->month)), ' ', $this->year;
        }
    ?>
</h2>
<form action="" method="get">
    <?php if (!empty($this->error)): ?>
        <p class="error"><?= $this->error ?></p>
    <?php endif ?>
    <p>
        <label class="games-date">
            <input type="radio" name="type" value="month"<?= $this->type == 'month' || empty($this->type) ? ' checked' : '' ?>>
            By month:
        </label>
        <select name="month" class="games-date">
            <?php for ($m = 1; $m <= 12; $m++): ?>
            <option value="<?= $m ?>"<?= $this->month == $m ? ' selected' : '' ?>><?= date('F', mktime(0, 0, 0, $m)) ?></option>
            <?php endfor ?>
        </select>
        <select name="year" class="games-date">
            <?php for ($y = date('Y'), $l = $y - 5; $y > $l; $y--): ?>
            <option<?= $this->year == $y ? ' selected' : '' ?>><?= $y ?></option>
            <?php endfor ?>
        </select>
        <input class="games-date-submit" type="submit" value=" &nbsp; OK &nbsp; ">
        &emsp;
        &emsp;
        &emsp;
        <label class="games-date">
            <input type="radio" name="type" value="date"<?= $this->type == 'date' ? ' checked' : '' ?>>
            By dates:
        </label>
        <input class="games-date" type="date" size="8" name="from" value="<?= $this->dateFrom ?>">
        <input class="games-date" type="date" size="8" name="to" value="<?= $this->dateTo ?>">
        <input class="games-date-submit" type="submit" value=" &nbsp; OK &nbsp; ">
    </p>
</form>
<?php $this->players->render() ?>
