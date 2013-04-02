<?php
function get_years_from_db()
{
	
}

function get_months_from_db()
{
	
}

function get_maps_from_db()
{
	
}

function create_selector($list)
{
	$count = 0;
	$html = '\n<select>';
	foreach ($list as $item)
		$html = $html . '\n\t<option value="' . $count++ . '">' . $item . '</option>';
	$html = $html . '\n</select>';
	return $html;
}
?>
<html>
<body>

<form action="welcome.php" method="post">
Year: <?php create_selector(get_years_from_db()) ?>
Month: <?php create_selector(get_months_from_db()) ?>
Map: <?php create_selector(get_maps_from_db()) ?>
<input type="submit">
</form>

</body>
</html>