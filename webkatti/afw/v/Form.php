<?php /* @var $this \afw\c\Form */ ?>
<form action="<?= $this->action ?>" method="<?= $this->method ?>"<?php if (!empty($this->maxFileSize)): ?> enctype="multipart/form-data"<?php endif ?>>
    <?php if (!empty($this->maxFileSize)): ?><input type="hidden" name="MAX_FILE_SIZE" value="<?= $this->maxFileSize ?>" /><?php endif ?>
    <?php if (!empty($this->error)): ?>
        <p class="error"><?= $this->error ?></p>
    <?php endif ?>

    <?php if (!empty($this->complete)): ?>
        <p class="complete"><?= $this->complete ?></p>
    <?php endif ?>

    <?= $this->contents() ?>
</form>
