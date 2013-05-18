<?php /* @var $this \afw\c\Form\FieldSelect */ ?>
<label>
    <?=$this->label?>
    <select name="<?=$this->name?>">
        <?php foreach ($this->options as $i=>$v) $this->renderOption($i, $v) ?>
    </select>
</label>
