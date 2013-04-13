<?php include_once 'heading.php'; ?>
<!DOCTYPE html>
<html>
<head>
<link rel="stylesheet" type="text/css" href="katina.css"/>
<script type="text/javascript">
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

<table>
<thead>
<tr>
<td>Player</td>
<td> </td>
<td>Player</td>
<td>Killed</td>
<td> </td>
<td>Killed By</td>
</tr>
</thead>
<?php
if($form_map)
{
	$ovo = array();
	$ovo['+'] = array();
	$ovo['-'] = array();
	$names = array();
	
	$names[$guid] = '&lt;unknown&gt;';
	
	if(($games = get_game_ids_from_db($con, $form_year, $form_month, $form_map)))
	{
		foreach($games as $game_id)
		{
			if(($ovo1 = get_ovo_from_db($con, $game_id, $guid)))
			{
				foreach($ovo1['+'] as $guid2 => $count)
				{
					if(!isset($ovo['+'][$guid2]))
						$ovo['+'][$guid2] = 0;
					$ovo['+'][$guid2] += $count;
					$names[$guid2] = '&lt;unknown&gt;';
				}
				foreach($ovo1['-'] as $guid2 => $count)
				{
					if(!isset($ovo['-'][$guid2]))
						$ovo['-'][$guid2] = 0;
					$ovo['-'][$guid2] += $count;
					$names[$guid2] = '&lt;unknown&gt;';
				}
			}
		}
	}

	add_names_to_guids_from_db($con, $names);
	
	foreach($names as $guid2 => $name)
	{
		$kill = isset($ovo['+'][$guid2]) ? $ovo['+'][$guid2] : 0;
		$killby = isset($ovo['-'][$guid2]) ? $ovo['-'][$guid2] : 0;
?>
<tr>
<td><?php echo oa_to_HTML($names[$guid]); ?> </td>
<td> vs </td>
<td><?php echo oa_to_HTML($name); ?> </td>
<td><?php echo $kill; ?> </td>
<td><?php echo ($kill == $killby) ? '=' : ($kill < $killby) ? '&lt;' : '&gt;' ?>
<td><?php echo $killby; ?> </td>
</tr>
<?php
		
	}
}
?>
</table>
</body>
</html>
<?php mysqli_close($con); ?>
