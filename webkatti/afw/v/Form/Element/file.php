<?php /* @var $this \afw\c\Form\FieldFile */ ?>
<label>
	<?=$this->label?>
	<input type="file" name="<?=$this->name?>" />
</label>
<?php if (!empty($this->value)): ?>
<small class="description"><a href="/<?=$this->src?>">/<?=$this->src?></a></small>
<label>
	<input type="checkbox" name="<?=$this->name?>-delete" value="1" /> <?=$this->labelDelete?>
</label>
<?php endif ?>