<h3>Last month's stats for player</h3>
<h2>
    <?= wk\Utils::colorecho($this->name) ?>
    <?php if ($this->registered): ?>
        <span class="registered">
            registered
        </span>
    <?php endif ?>
</h2>

<?php if (M::supervisor()->isAuthed()): ?>
    <h4>Names</h4>
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
    <h4>Numbers</h4>
    
    <div style="display: inline-block; width: 33%;">
        <table>
            <tbody>
                <tr>
                    <td>Flag carriers fragged:</td>
                    <td>?</td>
                </tr>
                <tr>
                    <td>Deaths as flag carrier:</td>
                    <td>?</td>
                </tr>
                <tr>
                    <td>Spawnkills done:</td>
                    <td><?= number_format(@$this->stats['spawnKillsDone']) ?></td>
                </tr>
                <tr>
                    <td>Spawnkills taken:</td>
                    <td><?= number_format(@$this->stats['spawnKillsRecv']) ?></td>
                </tr>
            </tbody>
        </table>
    </div>
    
    <div style="display: inline-block; width: 33%;">
        <table>
            <tbody>
                <tr>
                    <td>Holy-Shit frags:</td>
                    <td>?</td>
                </tr>
                <tr>
                    <td>Holy-Shit deaths:</td>
                    <td>?</td>
                </tr>
                <tr>
                    <td>Railgun pushes done:</td>
                    <td><?= number_format(@$this->stats['pushesDone']) ?></td>
                </tr>
                <tr>
                    <td>Railgun pushes taken:</td>
                    <td><?= number_format(@$this->stats['pushesRecv']) ?></td>
                </tr>
            </tbody>
        </table>
    </div>
    
    <div style="display: inline-block; width: 33%;">
        <table>
            <tbody>
                <tr>
                    <td>Health picked up:</td>
                    <td><?= number_format(@$this->stats['healthPickedUp']) ?></td>
                </tr>
                <tr>
                    <td>Armor picked up:</td>
                    <td><?= number_format(@$this->stats['armorPickedUp']) ?></td>
                </tr>
                <tr>
                    <td>&nbsp;</td>
                    <td></td>
                </tr>
                <tr>
                    <td>&nbsp;</td>
                    <td></td>
                </tr>
            </tbody>
        </table>
    </div>


    <?php if (!empty($this->weapons)): ?>
        <h4>Weapons</h4>
        <table>
            <thead>
                <th>Weapon</th>
                <th>Frags</th>
                <th>Deaths</th>
                <th>Shots</th>
                <th>Hits</th>
                <th>Dmg Done</th>
                <th>Hits Taken</th>
                <th>Dmg Taken</th>
                <th>Dmg Ratio</th>
                <th>Accuracy</th>
            </thead>
            <tbody>
                <?php foreach ($this->weapons as $weap => $weapon): ?>
                    <tr>
                        <td><?= wk\Utils::$weapons[$weap] ?></td>
                        <td><span class="psGood"><?= number_format(@$weapon['killCount']) ?></span> <small>(<?= @number_format(@$weapon['killsPercent'] * 100, 1) ?> %)</small></td>
                        <td><span class="psBad"><?= number_format(@$weapon['deathCount']) ?></span> <small>(<?= @number_format(@$weapon['deathsPercent'] * 100, 1) ?> %)</small></td>
                        <td class="psShots"><?= number_format(@$weapon['shots']) ?></td>
                        <td class="psDone"><?= number_format(@$weapon['hits']) ?></td>
                        <td><span class="psDone"><?= number_format(@$weapon['dmgDone']) ?></span> <small>(<?= @number_format(@$weapon['dmgDonePercent'] * 100, 1) ?> %)</small></td>
                        <td class="psRecv"><?= number_format(@$weapon['hitsRecv']) ?></td>
                        <td><span class="psRecv"><?= number_format(@$weapon['dmgRecv']) ?></span> <small>(<?= @number_format(@$weapon['dmgRecvPercent'] * 100, 1) ?> %)</small></td>
                        <td><span class="<?= @$weapon['dmgRatio']<1 ? 'psBad' : 'psGood' ?>"><?= number_format(@$weapon['dmgRatio'] * 100, 1) ?> %<span></td>
                        <td class="psShots"><?= number_format(@$weapon['accuracy'] * 100, 1) ?> %</td>
                    </tr>
                <?php endforeach ?>
            </tbody>
            <tfoot>
                <tr>
                    <td>Total</td>
                    <td class="psGood"><?= number_format(@$this->weaponsTotal['killCount']) ?></td>
                    <td class="psBad"><?= number_format(@$this->weaponsTotal['deathCount']) ?></td>
                    <td class="psShots"><?= number_format(@$this->weaponsTotal['shots']) ?></td>
                    <td class="psDone"><?= number_format(@$this->weaponsTotal['hits']) ?></td>
                    <td class="psDone"><?= number_format(@$this->weaponsTotal['dmgDone']) ?></td>
                    <td class="psRecv"><?= number_format(@$this->weaponsTotal['hitsRecv']) ?></td>
                    <td class="psRecv"><?= number_format(@$this->weaponsTotal['dmgRecv']) ?></td>
                    <td><span class="<?= @$this->weaponsTotal['dmgRatio']<1 ? 'psBad' : 'psGood' ?>"><?= number_format(@$this->weaponsTotal['dmgRatio'] * 100, 1) ?> %</span></td>
                    <td class="psShots"><?= number_format(@$this->weaponsTotal['accuracy'] * 100, 1) ?> %</td>
                </tr>
            </tfoot>
        </table>
    <?php endif ?>
        
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
                        <td><a href="<?= wk\c\Link::player($guid) ?>"><?= wk\Utils::colorecho($ovo['name']) ?></a></td>
                        <td class="psGood"><?= $ovo['kills'] ?></td>
                        <td class="psBad"><?= $ovo['deaths'] ?></td>
                        <td class="<?= $ovo['kd']<100 ? 'psBad' : 'psGood' ?>"><?= $ovo['kd'] ?> %</td>
                    </tr>
                <?php endforeach ?>
            </tbody>
        </table>
    <?php endif ?>

    <h4>Games</h4>
    <?php $this->games->render() ?>
<?php endif ?>
