<?php
date_default_timezone_set('GMT');

$con = mysqli_connect("176.56.235.126", "oadb", "", "oadb");
if(mysqli_connect_errno($con))
	echo mysqli_connect_error();

$form_year = isset($_POST['year']) ? $_POST['year'] : date('Y'); 
$form_month = isset($_POST['month']) ? $_POST['month'] : date('M'); 
$form_map = isset($_POST['map']) ? $_POST['map'] : false; 


$months = array
(
	'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'	
);

function get_month_number($month)
{
	global $months;
	$count = 1;
	foreach($months as $m)
	{
		if($m == $month)
			return $count;
		++$count;
	}
	return false;
}

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

function create_selector($name, $list, $dflt)
{
	//echo "dflt: $dflt";
	//$count = 0;
	$html = "\n" . '<select name="' . $name . '">';
	foreach ($list as $item)
	{
		if($item == $dflt)
			$html = $html . "\n\t" . '<option value="' . $item . '" selected>' . $item . '</option>';
		else
			$html = $html . "\n\t" . '<option value="' . $item . '">' . $item . '</option>';
	}
	$html = $html . "\n" . '</select>' . "\n";
	return $html;
}
?>
<!DOCTYPE html>
<html>
<body>

<form action="query.php" method="post">
Year: <?php echo create_selector('year', get_years_from_db(), $form_year) ?>
Month: <?php echo create_selector('month', $months, $form_month) ?>
Map: <?php echo create_selector('map', get_maps_from_db(), $form_map) ?>
<input type="submit">
</form>

<?php
if($form_map)
{
	$syear = $form_year;
	$eyear = $syear;
	$smonth = get_month_number($form_month);
	$emonth = $smonth + 1;
	if($emonth > 12)
	{
		$emonth = 1;
		++$eyear;
	}
	
	$cond1 = '`date` >= TIMESTAMP(\'' . $syear . '-' . $smonth . '-01\')';
	$cond2 = '`date` < TIMESTAMP(\'' . $eyear . '-' . $emonth . '-01\')';
	$cond3 = '`map` = \'' . $form_map . '\'';
	$sql = 'select `game_id` from `game` where ' . $cond1 . ' and ' . $cond2 . ' and ' . $cond3;
	$games_result = mysqli_query($con, $sql);	
	if(!$games_result)
		echo mysqli_error($con);
	
	$players = array();
	$names = array();
	
	while($games_row = mysqli_fetch_array($games_result))
	{
		// KILLS
		$cond1 = '(`game_id` = \'' . $games_row[0] . '\')';
		$cond2 = '(`weap` = \'2\' or `weap` = \'10\')';
		$sql = 'select `guid`,`count` from `kills` where ' . $cond1 . ' and ' . $cond2;
		
		$result = mysqli_query($con, $sql);	
		if(!$result)
			echo mysqli_error($con);
		
		while($row = mysqli_fetch_array($result))
		{
			$names[] = $row[0];
			$players[$row[0]]['k'] += $row[1];
		}

		// DEATHS
		$sql = 'select `guid`,`count` from `deaths` where ' . $cond1 . ' and ' . $cond2;
		$result = mysqli_query($con, $sql);	
		if(!$result)
			echo mysqli_error($con);
		
		while($row = mysqli_fetch_array($result))
		{
			$names[] = $row[0];
			$players[$row[0]]['d'] += $row[1];
		}

		// CAPS
		$sql = 'select `guid`,`count` from `caps` where ' . $cond1;
		$result = mysqli_query($con, $sql);	
		if(!$result)
			echo mysqli_error($con);
		
		while($row = mysqli_fetch_array($result))
		{
			$names[] = $row[0];
			$players[$row[0]]['c'] += $row[1];
		}
	}
	// echo '$sql: ' . $sql;
	
// 	select game_id, date, map from game where date >= TIMESTAMP(year, month) and date < TIMESTAMP(year, month + 1
	
// 	foreach game_id as id
// 	select guid, count from kills where game_id == id
	
?>
<table>

<?php
foreach($players as $guid => $stats)
{
	echo $guid . ': ' . $stats['k'] . ' kills<br/>';
	echo $guid . ': ' . $stats['d'] . ' deaths<br/>';
	echo $guid . ': ' . $stats['c'] . ' caps<br/>';
	echo '<hr/><br/>';
}
?>

</table>
<?php
}
?>

</body>
</html>
<?php
mysqli_close($con);
?>
