<?php
date_default_timezone_set('GMT');
$con = mysqli_connect("176.56.235.126", "oadb", "", "oadb");
if(mysqli_connect_errno($con))
	echo mysqli_connect_error();

$form_year = isset($_POST['year']) ? mysqli_real_escape_string($con, $_POST['year']) : date('Y'); 
$form_month = isset($_POST['month']) ? mysqli_real_escape_string($con, $_POST['month']) : date('M'); 
$form_map = isset($_POST['map']) ? mysqli_real_escape_string($con, $_POST['map']) : false;
$form_sort = isset($_POST['sort']) ? $_POST['sort'] : 'name';

if(php_sapi_name() == 'cli')
{
	$form_year = '2013';
	$form_month = 'Apr';
	$form_map = 'oasago2';
}

$oatohtmltab = array
(
	"black"
	, "red"
	, "lime"
	, "yellow"
	, "blue"
	, "cyan"
	, "magenta"
	, "white"
);
function oa_to_HTML($msg)
{
	//echo "\nDEBUG: $msg\n";
	global $oatohtmltab;
	$oss = '<span style="color: white">';
	$l = strlen($msg);
	$i = 0;
	while($i < $l)
	{
		if($i < $l && $msg[$i] == '^' && ($i + 1) < $l && ctype_alnum($msg[$i + 1]))
		{
			$oss = $oss . '</span>';
			$code = (ord($msg[$i + 1]) - ord('0')) & 7;
			$oss = $oss . '<span style="color: ' . $oatohtmltab[$code] . '">';
			$i += 2;
		}
		else if($i < $l && ord($msg[$i]) < 32)
		{
			$oss . $oss . "#";
			$i++;
		}
		else
		{
			$oss = $oss . $msg[$i];
			$i++;
		}
	}

	$oss = $oss . '</span>' . "\n";

	return $oss;
}

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
	
	mysqli_free_result($result);
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
	
	mysqli_free_result($result);
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

function strip($name)
{
	//echo "$name<br/>\n";
	$stripped = '';
	for($i = 0; $i < strlen($name); $i++)
		if(ctype_alpha(ord($name[$i])))
			$stripped = $stripped . $name[$i];
	//echo "$stripped<br/>\n";
	return $stripped;
}

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
?>
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
<span class="monthly-map-form-heading">Year:</span> <?php echo create_selector('year', get_years_from_db(), $form_year) ?>
<span class="monthly-map-form-heading">Month:</span> <?php echo create_selector('month', $months, $form_month) ?>
<span class="monthly-map-form-heading">Map:</span> <?php echo create_selector('map', get_maps_from_db(), $form_map) ?>
<input id="monthly-map-form-sort" name="sort" type="hidden" value="name"/>
<input type="submit"/>
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
	mysqli_free_result($games_result);
	
	if(count($names) > 0)
	{
		// populate $names
		$sql = 'select * from `player` where';
		$sep = ' ';
		foreach($names as $guid => $name)
		{
			$sql = $sql . $sep . '`guid` = \'' . $guid . '\'';
			$sep = ' or ';
		}
		
		$sql = $sql . ' order by `count` desc';
		
		$result = mysqli_query($con, $sql);
		if(!$result)
			echo mysqli_error($con);
		
		while($row = mysqli_fetch_array($result))
		{
			$names[$row[0]] = $row[1];
		}
		
		mysqli_free_result($result);
	}	
	
	$table = array();
	foreach($players as $guid => $stats)
	{
		$kd = $stats['d'] == 0 ? ($stats['k'] ? 'perf' : 0) : $stats['k'] / $stats['d'];
		$cd = $stats['d'] == 0 ? ($stats['c'] ? 'perf' : 0) : ($stats['c'] * 100) / $stats['d'];

		$table[] = array
		(
			'name' => oa_to_HTML($names[$guid])
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
<table id="score-table">
<tr id="score-table-header">
	<td class="score-table-heading">Player <a href="#" onclick="sort_by('name');">[sort]</a></td>
	<td class="score-table-heading">Kills/Death <a href="#" onclick="sort_by('kd');">[sort]</a></td>
	<td class="score-table-heading">100 x Caps/Deaths <a href="#" onclick="sort_by('cd');">[sort]</a></td>
</tr>
<?php
$odd = true;
foreach($table as $stats)
{
	$tr_class = $odd ? "score-table-tr-odd" : "score-table-tr-even";
	$td_class = $odd ? "score-table-td-odd" : "score-table-td-even";
	echo "<tr class=\"" . $tr_class . "\">\n";
	echo "<td class=\"" . $td_class . "-name\">" . $stats['name'] . "</td>";
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
