<?php /* @var $this \wk\c\PlayerStats */ ?>
<table>
    <thead>
        <tr>
            <th title="Player Name">Name</th>
            <th title="Time spent ingame">H:M:S</th>
            <th title="Fragged other players">Frags</th>
            <th title="Killed by other players">Deaths</th>
            <th title="Frag/Death Ratio">F / D</th>
            <th title="Weapon Accuracy">Acc</th>
            <th title="Average Time between Frags">S / F</th>
            <th title="Flag Captures">Caps</th>
            <th title="Average Captures per Death * 100">C / D</th>
            <th title="Average Captures per Hour">C / H</th>
            <th title="√ (Frags * Caps) / Respawns %">Agility</th>
            <th title="√ (Frags * Caps) / Hours">Speed</th>
            <th title="(Frags * Caps) / (Respawns * Hours)">Power</th>
        </tr>
    </thead>
    <tbody>
        <?php foreach ((array)@$this->players as $guid => $player): ?>
            <tr>
                <td><a href="<?= wk\c\Link::player($guid) ?>"><?php wk\Utils::colorecho($player['name']) ?></a></td>
                <td class="right" data-raw="<?= $player['time_raw'] ?>"><?= $player['time'] ?></td>
                <td class="right"><?= $player['kills'] ?></td>
                <td class="right"><?= $player['deaths'] ?></td>
                <td class="right"><?= $player['kd'] ?></td>
                <td class="right"><?= $player['acc'] ?></td>
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
