<?php /* @var $this \wk\c\KDCD */ ?>
<table>
    <thead>
        <tr>
            <th>Name</th>
            <th>Frags</th>
            <th>Caps</th>
            <th>Respawns</th>
            <th>F / R %</th>
            <th>C / R %</th>
            <th>F / R * C / R %</th>
        </tr>
    </thead>
    <tbody>
        <?php foreach ((array)@$this->players as $guid => $player): ?>
            <tr>
                <td><a href="/player-<?= $guid ?>"><?php wk\Utils::colorecho($player['name']) ?></a></td>
                <td><?= $player['kills'] ?></td>
                <td><?= $player['caps'] ?></td>
                <td><?= $player['deaths'] ?></td>
                <td><?= $player['kd'] ?></td>
                <td><?= $player['cd'] ?></td>
                <td><?= $player['kdcd'] ?></td>
            </tr>
        <?php endforeach ?>
    </tbody>
</table>
