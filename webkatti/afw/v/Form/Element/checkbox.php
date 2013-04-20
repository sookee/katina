<?php /* @var $this \afw\c\Form\Field */ ?>
<label>
	<input type="checkbox" name="<?=$this->name?>" value="1"<?= !empty($this->value) ? ' checked="checked"' : '' ?> />
	<?=$this->label?>
</label>
