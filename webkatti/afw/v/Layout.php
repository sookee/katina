<?php /* @var $this \afw\c\Layout */ ?>
<!doctype html>
<html>
<head>
	<title><?=htmlspecialchars($this->title)?></title>
	<meta charset="utf-8">
<?php if (!empty($this->keywords)): ?>
	<meta name="keywords" content="<?=htmlspecialchars($this->keywords)?>">
<?php endif ?>
<?php if (!empty($this->description)): ?>
	<meta name="description" content="<?=htmlspecialchars($this->description)?>">
<?php endif ?>
<?php $this->css() ?>
<?php $this->js() ?>
<?php if (isset($this->head)): ?>
	<?=$this->head?>
<?php endif ?>
</head>
<body><?php $this->contents() ?></body>
</html>
