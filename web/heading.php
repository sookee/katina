<?php
session_start();

date_default_timezone_set('GMT');

$con = mysqli_connect("176.56.235.126", "oadb", "", "oadb");
if(mysqli_connect_errno($con))
	echo mysqli_connect_error();

$form_year = isset($_POST['year']) ? mysqli_real_escape_string($con, $_POST['year']) : date('Y'); 
$form_month = isset($_POST['month']) ? mysqli_real_escape_string($con, $_POST['month']) : date('M'); 
$form_map = isset($_POST['map']) ? mysqli_real_escape_string($con, $_POST['map']) : false;
$form_sort = isset($_POST['sort']) ? $_POST['sort'] : 'name';
$guid = isset($_GET['guid']) ? mysqli_real_escape_string($con, $_GET['guid']) : false;

if(php_sapi_name() == 'cli')
{
	$form_year = '2013';
	$form_month = 'Apr';
	$form_map = 'oan';
	$guid = 'E20DDC17';
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

function get_years_from_db($con)
{
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

function get_maps_from_db($con)
{
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

function get_game_ids_from_db($con, $year, $month, $mapname)
{
	$syear = $year;
	$eyear = $syear;
	$smonth = get_month_number($month);
	$emonth = $smonth + 1;
	if($emonth > 12)
	{
		$emonth = 1;
		++$eyear;
	}
	
	$cond1 = '`date` >= TIMESTAMP(\'' . $syear . '-' . $smonth . '-01\')';
	$cond2 = '`date` < TIMESTAMP(\'' . $eyear . '-' . $emonth . '-01\')';
	$cond3 = '`map` = \'' . $mapname . '\'';
	
	$sql = 'select `game_id` from `game` where ' . $cond1 . ' and ' . $cond2 . ' and ' . $cond3;
	$result = mysqli_query($con, $sql);	
	if(!$result)
	{
		echo mysqli_error($con);
		return false;
	}

	$ids = array();
	
	while($row = mysqli_fetch_array($result))
		$ids[] = $row[0];
	
	mysqli_free_result($result);

	return $ids;
}

/**
 * 
 * @param con = database connection
 * @param $names assoc array { 'guid' => '' }
 * @return true on success and $names = { 'guid' => 'name' }
 */
function add_names_to_guids_from_db($con, &$names)
{
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
		{
			echo mysqli_error($con);
			return false;
		}
		
		while($row = mysqli_fetch_array($result))
		{
			$names[$row[0]] = $row[1];
		}
		
		mysqli_free_result($result);
	}
	return true;
}

/**
 * 
 * @param unknown $con
 * @param unknown $game_id
 * @param unknown $guid1
 * @param unknown $guid1
 * @return multitype:unknown
 */
function get_ovo_from_db($con, $game_id, $guid)
{
	$ovo = array();
	$ovo['+'] = array();
	$ovo['-'] = array();
	$result = mysqli_query($con, 'select `guid1`,`guid2`,`count` from `ovo` where `game_id` = \''
			. $game_id . '\'');
	if(!$result)
	{
		echo mysqli_error($con);
		return false;
	}
	
// 	`game_id` int(4) unsigned NOT NULL,
// 	`guid1` varchar(8) NOT NULL,
// 	`guid2` varchar(8) NOT NULL,
// 	`count` int(2) unsigned NOT NULL DEFAULT '0'
	
	while($row = mysqli_fetch_array($result))
	{
		if($row[0] == $guid)
		{
			if(!isset($ovo['+'][$row[1]]))
				$ovo['+'][$row[1]] = 0;
			$ovo['+'][$row[1]] += $row[2];
		}

		if($row[1] == $guid)
		{
			if(!isset($ovo['-'][$row[0]]))
				$ovo['-'][$row[0]] = 0;
			$ovo['-'][$row[0]] += $row[2];
		}
	}

	mysqli_free_result($result);

	return $ovo;
}
?>
