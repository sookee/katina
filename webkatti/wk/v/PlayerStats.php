<?php /* @var $this \wk\c\PlayerStats */ ?>
<table>
    <thead>
        <tr>
            <th>Name</th>
            <th>H:M:S</th>
            <th>Respawns</th>
            <th>Frags</th>
            <th>F / R %</th>
            <th>S / F</th>
            <th>Caps</th>
            <th>C / R %</th>
            <th>C / H</th>
            <th title="√ (Frags * Caps) / Respawns %">Agility</th>
            <th title="√ (Frags * Caps) / Hours">Speed</th>
            <th title="(Frags * Caps) / (Respawns * Hours)">Power</th>
        </tr>
    </thead>
    <tbody>
        <?php foreach ((array)@$this->players as $guid => $player): ?>
            <tr>
                <td><a href="<?= wk\c\Link::player($guid) ?>"><?php wk\Utils::colorecho($player['name']) ?></a></td>
                <td class="right"><?= $player['time'] ?></td>
                <td class="right"><?= $player['deaths'] ?></td>
                <td class="right"><?= $player['kills'] ?></td>
                <td class="right"><?= $player['kd'] ?></td>
                <td class="right"><?= $player['tk'] ?></td>
                <td class="right"><?= $player['caps'] ?></td>
                <td class="right"><?= $player['cd'] ?></td>
                <td class="right"><?= $player['ct'] ?></td>
                <td class="right"><?= $player['kcd'] ?></td>
                <td class="right"><?= $player['kct'] ?></td>
                <td class="right"><?= $player['kcdt'] ?></td>
            </tr>
        <?php endforeach ?>
    </tbody>
</table>

<p>
    Enabled filters:
    sum time > <?= round($this->minTime / 60) ?> min,
    sum respawns > <?= $this->minDeaths ?>
</p>
