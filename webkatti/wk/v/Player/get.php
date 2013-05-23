<h3>Recent stats for player</h3>
<h2>
    <?= wk\Utils::colorecho($this->name) ?>
    <?php if ($this->registered): ?>
        <span class="registered">
            registered
        </span>
    <?php endif ?>
</h2>
Period: <?= $this->beginDate ?> - now<br /><br />

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
    

    <div class="col33 coll">
        <table>
            <tbody>
                <tr>
                    <td>Flag carriers fragged:</td>
                    <td class="y"><?= number_format(@$this->stats['carrierFrags']) ?></td>
                </tr>
                <tr>
                    <td>Deaths as flag carrier:</td>
                    <td class="y"><?= number_format(@$this->stats['carrierFragsRecv']) ?></td>
                </tr>
                <tr>
                    <td>Spawnkills done:</td>
                    <td class="y"><?= number_format(@$this->stats['spawnKillsDone']) ?></td>
                </tr>
                <tr>
                    <td>Spawnkills taken:</td>
                    <td class="y"><?= number_format(@$this->stats['spawnKillsRecv']) ?></td>
                </tr>
            </tbody>
        </table>
    </div>
    
    <div class="col33 colc">
        <table>
            <tbody>
                <tr>
                    <td>Holy-Shit frags:</td>
                    <td class="y"><?= number_format(@$this->stats['holyShitFrags']) ?></td>
                </tr>
                <tr>
                    <td>Holy-Shit deaths:</td>
                    <td class="y"><?= number_format(@$this->stats['holyShitFragged']) ?></td>
                </tr>
                <tr>
                    <td>Railgun pushes done:</td>
                    <td class="y"><?= number_format(@$this->stats['pushesDone']) ?></td>
                </tr>
                <tr>
                    <td>Railgun pushes taken:</td>
                    <td class="y"><?= number_format(@$this->stats['pushesRecv']) ?></td>
                </tr>
            </tbody>
        </table>
    </div>
    
    <div class="col33 colr">
        <table>
            <tbody>
                <tr>
                    <td>Health picked up:</td>
                    <td class="y"><?= number_format(@$this->stats['healthPickedUp']) ?></td>
                </tr>
                <tr>
                    <td>Armor picked up:</td>
                    <td class="y"><?= number_format(@$this->stats['armorPickedUp']) ?></td>
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
                <th title="Type of Weapon/Damage">Weapon</th>
                <th title="Frags done with Weapon">Frags</th>
                <th title="Deaths by Weapon">Deaths</th>
                <th title="Shots with Weapon">Shots</th>
                <th title="Hits with Weapon">Hits</th>
                <th title="Damage done with Weapon">Dmg Done</th>
                <th title="Hits taken from Weapon">Hits Taken</th>
                <th title="Damage taken from Weapon">Dmg Taken</th>
                <th title="Damage done/taken Ratio">Dmg Ratio</th>
                <th title="Weapon Accuracy (hits/shots)">Accuracy</th>
            </thead>
            <tbody>
                <?php foreach ($this->weapons as $weap => $weapon): ?>
                    <tr>
                        <td><?= wk\Utils::$weapons[$weap] ?></td>
                        <td><span class="g"><?= number_format(@$weapon['killCount']) ?></span> <small>(<?= @number_format(@$weapon['killsPercent'] * 100, 1) ?> %)</small></td>
                        <td><span class="r"><?= number_format(@$weapon['deathCount']) ?></span> <small>(<?= @number_format(@$weapon['deathsPercent'] * 100, 1) ?> %)</small></td>
                        <td class="y"><?= number_format(@$weapon['shots']) ?></td>
                        <td class="c"><?= number_format(@$weapon['hits']) ?></td>
                        <td><span class="c"><?= number_format(@$weapon['dmgDone']) ?></span> <small>(<?= @number_format(@$weapon['dmgDonePercent'] * 100, 1) ?> %)</small></td>
                        <td class="m"><?= number_format(@$weapon['hitsRecv']) ?></td>
                        <td><span class="m"><?= number_format(@$weapon['dmgRecv']) ?></span> <small>(<?= @number_format(@$weapon['dmgRecvPercent'] * 100, 1) ?> %)</small></td>
                        <td><span class="<?= @$weapon['dmgRatio']<0.995 ? 'r' : 'g' ?>"><?= number_format(@$weapon['dmgRatio'] * 100) ?> %<span></td>
                        <td class="y"><?= number_format(@$weapon['accuracy'] * 100) ?> %</td>
                    </tr>
                <?php endforeach ?>
            </tbody>
            <tfoot>
                <tr>
                    <td>Total</td>
                    <td class="g"><?= number_format(@$this->weaponsTotal['killCount']) ?></td>
                    <td class="r"><?= number_format(@$this->weaponsTotal['deathCount']) ?></td>
                    <td class="y"><?= number_format(@$this->weaponsTotal['shots']) ?></td>
                    <td class="c"><?= number_format(@$this->weaponsTotal['hits']) ?></td>
                    <td class="c"><?= number_format(@$this->weaponsTotal['dmgDone']) ?></td>
                    <td class="m"><?= number_format(@$this->weaponsTotal['hitsRecv']) ?></td>
                    <td class="m"><?= number_format(@$this->weaponsTotal['dmgRecv']) ?></td>
                    <td><span class="<?= @$this->weaponsTotal['dmgRatio']<0.995 ? 'r' : 'g' ?>"><?= number_format(@$this->weaponsTotal['dmgRatio'] * 100) ?> %</span></td>
                    <td class="y"><?= number_format(@$this->weaponsTotal['accuracy'] * 100) ?> %</td>
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
                    <th>Games</th>
                    <th>Frags</th>
                    <th>Deaths</th>
                    <th>F / D</th>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($this->ovos as $guid => $ovo): ?>
                    <tr>
                        <td><a href="<?= wk\c\Link::player($guid) ?>"><?= wk\Utils::colorecho($ovo['name']) ?></a></td>
                        <td class="y"><?= $ovo['games'] ?></td>
                        <td class="g"><?= $ovo['kills'] ?></td>
                        <td class="r"><?= $ovo['deaths'] ?></td>
                        <td class="<?= $ovo['kd']<100 ? 'r' : 'g' ?>"><?= $ovo['kd'] ?> %</td>
                    </tr>
                <?php endforeach ?>
            </tbody>
        </table>
    <?php endif ?>

    <h4>Games</h4>
    <?php $this->games->render() ?>
<?php endif ?>
