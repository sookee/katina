<?php /* @var $this \afw\c\Form\Field */ ?>
<?php if (!empty($this->exception)): ?>
    <p class="error"><?= $this->exception instanceof \Exception ? $this->exception->getMessage() : $this->exception ?></p>
<?php endif ?>
<?php $this->contents() ?>
<?php if (!empty($this->description)): ?>
    <small class="description"><?=$this->description?></small>
<?php endif ?>
