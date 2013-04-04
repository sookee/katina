<?php
date_default_timezone_set('GMT');
$con = mysqli_connect("176.56.235.126", "oadb", "", "oadb");
if(mysqli_connect_errno($con))
	echo mysqli_connect_error();

$form_year = isset($_POST['year']) ? $_POST['year'] : date('Y'); 
$form_month = isset($_POST['month']) ? $_POST['month'] : date('M'); 
$form_map = isset($_POST['map']) ? $_POST['map'] : false;

if(php_sapi_name() == 'cli')
{
	$form_year = '2013';
	$form_month = 'Apr';
	$form_map = 'oasago2';
}

#define ColorIndex(c)	( ( (c) - '0' ) & 7 )
#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && isalnum(*((p)+1)) ) // ^[0-9a-zA-Z]
#define Q_IsSpecialChar(c) ((c) && ((c) < 32))

$oatoirctab = array
(
	1 // "black"
	, 4 // "red"
	, 3 // "lime"
	, 8 // "yellow"
	, 2 // "blue"
	, 12 // "cyan"
	, 6 // "magenta"
	, 0 // "white"
);

$IRC_BOLD = "";
$IRC_NORMAL = "";
$IRC_COLOR = "";

function oa_to_IRC($msg)
{
	global $oatoirctab;
	$oss = $IRC_BOLD;
	$l = strlen($msg);
	$i = 0;
	while($i < $l)
	{
		if($i < $l && $msg[$i] == '^' && $msg[$i + 1] && ctype_alnum($msg[$i + 1]))
		{
			$oss = $oss . $IRC_NORMAL;
			//$code = $msg[$i + 1] % 8;
			$code = ( ( ($msg[$i]) - '0' ) & 7 );
			$oss = $oss . $IRC_COLOR . ($oatoirctab[$code] < 10 ? "0" : "") . $oatoirctab[$code];
			$i += 2;
		}
		else if($msg[$i] && $msg[$i] < 32)
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

	$oss = $oss . $IRC_NORMAL;

	return $oss;
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
<body bgcolor="#222222">

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
			$names[$row[0]] = '<unknown>';
			$players[$row[0]]['k'] += $row[1];
		}

		// DEATHS
		$sql = 'select `guid`,`count` from `deaths` where ' . $cond1 . ' and ' . $cond2;
		$result = mysqli_query($con, $sql);	
		if(!$result)
			echo mysqli_error($con);
		
		while($row = mysqli_fetch_array($result))
		{
			$names[$row[0]] = '<unknown>';
			$players[$row[0]]['d'] += $row[1];
		}

		// CAPS
		$sql = 'select `guid`,`count` from `caps` where ' . $cond1;
		$result = mysqli_query($con, $sql);	
		if(!$result)
			echo mysqli_error($con);
		
		while($row = mysqli_fetch_array($result))
		{
			$names[$row[0]] = '&lt;unknown&gt;';
			$players[$row[0]]['c'] += $row[1];
		}
	}

	// populate $names
	$sql = 'select * from `player` where';
	$sep = ' ';
	foreach($names as $guid => $name)
	{
		$sql = $sql . $sep . '`guid` = \'' . $guid . '\'';
		$sep = ' or ';
	}
	
	$sql = $sql . ' order by `count`';
	
	$result = mysqli_query($con, $sql);
	if(!$result)
		echo mysqli_error($con);
	
	while($row = mysqli_fetch_array($result))
	{
		$names[$row[0]] = $row[1];
	}	
?>
<table>
<?php
foreach($players as $guid => $stats)
{
	$kd = $stats['k'] / $stats['d'];
	$cd = ($stats['c'] * 100) / $stats['d'];
	echo "<tr>\n";
	echo "<td>" . oa_to_HTML($names[$guid]) . "</td>";
	echo "<td>" . $kd . "</td>";
	echo "<td>" . $cd . "</td>";
	echo "</tr>\n";
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
