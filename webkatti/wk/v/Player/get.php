<h2>
    <?= wk\Utils::colorecho($this->name) ?>
    <?php if ($this->registered): ?>
        <span class="registered">
            registered
        </span>
    <?php endif ?>
</h2>

<?php if (M::supervisor()->isAuthed()): ?>
    <h3>Names</h3>
    <table>
        <thead>
            <tr>
                <th>Name</th>
                <th>Count</th>
                <th>Last used</th>
            </tr>
        </thead>
        <tbody>
            <?php foreach ($this->players as $player): ?>
                <tr>
                    <td><?= wk\Utils::colorecho($player['name']) ?></td>
                    <td><?= $player['count'] ?></td>
                    <td><?= afw\Utils::datef('d.m.Y H:i', $player['date']) ?></td>
                </tr>
            <?php endforeach ?>
        </tbody>
    </table>
<?php endif ?>

<?php if (!$this->games->isEmpty()): ?>
    <h3>Last month</h3>

    <?php if (!empty($this->ovos)): ?>
        <h4>Opponents</h4>
        <table>
            <thead>
                <tr>
                    <th>Player</th>
                    <th>Frags</th>
                    <th>Respawns</th>
                    <th>F / R %</th>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($this->ovos as $guid => $ovo): ?>
                    <tr>
                        <td><a href="/player-<?= $guid ?>"><?= wk\Utils::colorecho($ovo['name']) ?></a></td>
                        <td><?= $ovo['kills'] ?></td>
                        <td><?= $ovo['deaths'] ?></td>
                        <td><?= $ovo['kd'] ?></td>
                    </tr>
                <?php endforeach ?>
            </tbody>
        </table>
    <?php endif ?>

    <?php if (!empty($this->weapons)): ?>
        <h4>Weapons</h4>
        <table>
            <thead>
                <th>Weapon</th>
                <th>Frags %</th>
                <th>Respawns %</th>
            </thead>
            <tbody>
                <?php foreach ($this->weapons as $weap => $weapon): ?>
                    <tr>
                        <td><?= wk\Utils::$weapons[$weap] ?></td>
                        <td><?= @$weapon['kills'] ?></td>
                        <td><?= @$weapon['deaths'] ?></td>
                    </tr>
                <?php endforeach ?>
            </tbody>
        </table>
    <?php endif ?>

    <h4>Games</h4>
    <?php $this->games->render() ?>
<?php endif ?>
