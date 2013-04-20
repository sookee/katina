<table>
    <thead>
        <tr>
            <th>Server</th>
            <th>Time</th>
            <th>Map</th>
            <th></th>
        </tr>
    </thead>
    <tbody>
        <?php foreach ($this->items as $game): ?>
            <tr>
                <td><?= wk\Utils::server(long2ip($game['host']), $game['port']) ?></td>
                <td><?= afw\Utils::datef('d.m.Y H:i', $game['date']) ?></td>
                <td><?= $game['map'] ?></td>
                <td><a href="/game-<?= $game['game_id'] ?>">View</a></td>
            </tr>
        <?php endforeach ?>
    </tbody>
</table>
