<h2>
    <?= afw\Utils::datef('d.m.Y H:i', $this->game['date']) ?>
    <em>:</em>
    <?= $this->game['map'] ?>
    <em>@</em>
    <?= wk\Utils::server(long2ip($this->game['host']), $this->game['port']) ?>
</h2>

<?php $this->players->render() ?>
