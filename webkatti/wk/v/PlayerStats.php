<?php /* @var $this \wk\c\PlayerStats */ ?>
<table>
    <thead>
        <tr>
            <th>Name</th>
            <th>H:M:S</th>
            <th>Respawns</th>
            <th>Frags</th>
            <th>S / F</th>
            <th>F / R %</th>
            <th>Caps</th>
            <th>C / H</th>
            <th>C / R %</th>
            <th>F / R * C / R %</th>
        </tr>
    </thead>
    <tbody>
        <?php foreach ((array)@$this->players as $guid => $player): ?>
            <tr>
                <td><a href="<?= wk\c\Link::player($guid) ?>"><?php wk\Utils::colorecho($player['name']) ?></a></td>
                <td class="right"><?= $player['time'] ?></td>
                <td class="right"><?= $player['deaths'] ?></td>
                <td class="right"><?= $player['kills'] ?></td>
                <td class="right"><?= $player['tk'] ?></td>
                <td class="right"><?= $player['kd'] ?></td>
                <td class="right"><?= $player['caps'] ?></td>
                <td class="right"><?= $player['ct'] ?></td>
                <td class="right"><?= $player['cd'] ?></td>
                <td class="right"><?= $player['kdcd'] ?></td>
            </tr>
        <?php endforeach ?>
    </tbody>
</table>
