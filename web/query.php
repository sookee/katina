<?php
$con = mysqli_connect("176.56.235.126","oadb","","oadb");

// Check connection
if(mysqli_connect_errno($con))
{
	echo "Failed to connect to MySQL: " . mysqli_connect_error();
}

$months = array
(
	'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'	
);

function get_years_from_db()
{
	global $con;
	$items = array();
	$result = mysqli_query($con, 'select `date` from `game`');
	while($row = mysqli_fetch_array($result))
	{
		$parts = date_parse($row['date']);
		if(!in_array($parts['year'], $items))
			$items[] = $parts['year'];
	}
	sort($items);
	return $items;
}

function get_maps_from_db()
{
	global $con;
	$items = array();
	$result = mysqli_query($con, 'select `map` from `game`');
	while($row = mysqli_fetch_array($result))
	{
		if(!in_array($row['map'], $items))
			$items[] = $row['map'];
	}
	sort($items);
	return $items;
}

function create_selector($name, $list)
{
	$count = 0;
	$html = "\n" . '<select name="' . $name . '">';
	foreach ($list as $item)
		$html = $html . "\n\t" . '<option value="' . $count++ . '">' . $item . '</option>';
	$html = $html . "\n" . '</select>' . "\n";
	return $html;
}
?>
<!DOCTYPE html>
<html>
<body>

<form action="welcome.php" method="post">
Year: <?php echo create_selector('year', get_years_from_db()) ?>
Month: <?php echo create_selector('month', $months) ?>
Map: <?php echo create_selector('map', get_maps_from_db()) ?>
<input type="submit">
</form>

</body>
</html>
<?php
mysqli_close($con);
?>
