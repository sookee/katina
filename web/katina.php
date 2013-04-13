<?php include_once 'heading.php'; ?>
<!DOCTYPE html>
<html>
<head>
<link rel="stylesheet" type="text/css" href="katina.css"/>
<script type="text/javascript">
function sort_by(type)
{
	var form = document.getElementById('monthly-map-form');
	var sort = document.getElementById('monthly-map-form-sort');
	sort.value = type;
	form.submit();
}
</script>
</head>
<body>

<form id="monthly-map-form" action="katina.php" method="post">
<table id="form-table">
<tr id="form-table-header" class="table-header">
	<td class="table-heading">Year</td>
	<td class="table-heading">Month</td>
	<td class="table-heading">Map Name</td>
	<td><input id="monthly-map-form-sort" name="sort" type="hidden" value="name"/></td>
</tr>
<tr>
	<td><?php echo create_selector('year', get_years_from_db($con), $form_year) ?></td>
	<td><?php echo create_selector('month', $months, $form_month) ?></td>
	<td><?php echo create_selector('map', get_maps_from_db($con), $form_map) ?></td>
	<td><input type="submit"/></td>
</tr>
</table>
</form>
<?php
function cmp_names($a, $b)
{
	$name_a = strip($a['name']);
	$name_b = strip($b['name']);
	
	if($name_a < $name_b)
		return -1;
	if($name_a > $name_b)
		return 1;
	return 0;
}

function cmp_kd($a, $b)
{
	if($a['kd'] > $b['kd'])
		return -1;
	if($a['kd'] < $b['kd'])
		return 1;
	return 0;
	}

function cmp_cd($a, $b)
{
	if($a['cd'] > $b['cd'])
		return -1;
	if($a['cd'] < $b['cd'])
		return 1;
	return 0;
}

if($form_map)
{
	// Map Info
	$mapvotes = '&lt;unknown&gt;';
	$sql = 'select sum(`count`) from `votes` where `type` = \'map\' and `item` = \'' . $form_map . '\'';
	
	$result = mysqli_query($con, $sql);
	if(!$result)
		echo mysqli_error($con);
	else if($row = mysqli_fetch_array($result))
		$mapvotes = $row[0];
	
	mysqli_free_result($result);
	
	// Stats
	
	if(($ids = get_game_ids_from_db($con, $form_year, $form_month, $form_map)))
	{	
		$players = array();
		$names = array();
	
		foreach($ids as $id)
		{
			// KILLS
			$cond1 = '(`game_id` = \'' . $id . '\')';
			$cond2 = '(`weap` = \'2\' or `weap` = \'10\')';
			$sql = 'select `guid`,`count` from `kills` where ' . $cond1 . ' and ' . $cond2;
			
			$result = mysqli_query($con, $sql);	
			if(!$result)
				echo mysqli_error($con);
			
			while($row = mysqli_fetch_array($result))
			{
				$names[$row[0]] = '<unknown>';
				if(!isset($players[$row[0]]))
					$players[$row[0]] = array('k' => 0, 'd' => 0, 'c' => 0);
				$players[$row[0]]['k'] += $row[1];
			}
			
			mysqli_free_result($result);
	
			// DEATHS
			$sql = 'select `guid`,`count` from `deaths` where ' . $cond1 . ' and ' . $cond2;
			$result = mysqli_query($con, $sql);	
			if(!$result)
				echo mysqli_error($con);
			
			while($row = mysqli_fetch_array($result))
			{
				$names[$row[0]] = '<unknown>';
				if(!isset($players[$row[0]]))
					$players[$row[0]] = array('k' => 0, 'd' => 0, 'c' => 0);
				$players[$row[0]]['d'] += $row[1];
			}
			
			mysqli_free_result($result);
	
			// CAPS
			$sql = 'select `guid`,`count` from `caps` where ' . $cond1;
			$result = mysqli_query($con, $sql);	
			if(!$result)
				echo mysqli_error($con);
			
			while($row = mysqli_fetch_array($result))
			{
				$names[$row[0]] = '&lt;unknown&gt;';
				if(!isset($players[$row[0]]))
					$players[$row[0]] = array('k' => 0, 'd' => 0, 'c' => 0);
				$players[$row[0]]['c'] += $row[1];
			}
			
			mysqli_free_result($result);
		}
	}
	
	add_names_to_guids_from_db($con, $names);
	
	$table = array();
	foreach($players as $guid => $stats)
	{
		$kd = $stats['d'] == 0 ? ($stats['k'] ? 'perf' : 0) : $stats['k'] / $stats['d'];
		$cd = $stats['d'] == 0 ? ($stats['c'] ? 'perf' : 0) : ($stats['c'] * 100) / $stats['d'];

		$table[] = array
		(
			'guid' => $guid
			, 'name' => oa_to_HTML($names[$guid])
			, 'kd' => sprintf('%0.2f', $kd)
			, 'cd' => sprintf('%0.2f', $cd)
		);
	}
	
	switch ($form_sort)
	{
		case 'kd':
			usort($table, "cmp_kd");
			break;
		case 'cd':
			usort($table, "cmp_cd");
			break;
		default:
			usort($table, "cmp_names");
	}
?>
<table id="map-table">
<tr id="map-table-header" class="table-header">
	<td class="table-heading">Map Name</td>
	<td class="table-heading">Visitors</td>
	<td class="table-heading">Votes</td>
</tr>
<tr class="map-table-tr">
	<td class="map-table-td"><?php echo $form_map; ?></td>
	<td class="map-table-td"><?php echo count($players); ?></td>
	<td class="map-table-td"><?php echo $mapvotes; ?></td>
</tr>
</table>

<table id="score-table">
<tr id="score-table-header" class="table-header">
	<td class="table-heading">Player <a href="#" onclick="sort_by('name');">[sort]</a></td>
	<td class="table-heading">Kills/Death <a href="#" onclick="sort_by('kd');">[sort]</a></td>
	<td class="table-heading">100 x Caps/Deaths <a href="#" onclick="sort_by('cd');">[sort]</a></td>
</tr>
<?php
$odd = true;
foreach($table as $stats)
{
	$tr_class = $odd ? "score-table-tr-odd" : "score-table-tr-even";
	$td_class = $odd ? "score-table-td-odd" : "score-table-td-even";
	echo "<tr class=\"" . $tr_class . "\">\n";
	echo "<td class=\"" . $td_class . "-name\"><a class='a-player' href='player.php?guid=" . $stats['guid'] . "'>" . $stats['name'] . "</a></td>";
	echo "<td class=\"" . $td_class . "-kd\">" . $stats['kd'] . "</td>";
	echo "<td class=\"" . $td_class . "-cd\">" . $stats['cd'] . "</td>";
	echo "</tr>\n";
	$odd = !$odd;
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
