<?php
header("Content-type: text/xml");
date_default_timezone_set('GMT');

/**
 * format: generic <items><item/><item/><item/></items>
 *                 <items><item><key/><value/></item><item><key/><value/></item></items>
 */

function error($msg)
{
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo "<error>\n";
	echo "\t<msg>$msg</msg>\n";
	echo "<error>\n";
	exit(0);
}

function set_param($name, &$param, $dflt = false)
{
	// 0 -> no cli version
	$argmap = array
	(
		'get_bases' => 0, 'xsl' => 0, 'base' => 1, 'func' => 2, 'poll' => 3
	);
	
	$param = isset($_GET[$name]) ? $_GET[$name] : $dflt; 
	
	if(isset($argmap[$name]) && $argmap[$name] > 0 && php_sapi_name() == 'cli')
	{
		global $argc;
		global $argv;
		$gotc = $argc - 1;
		if($argc < $argmap[$name] + 1)
			error("Not enough arguments, expect: $argmap[$name] got: $gotc");
		$param = $argv[$argmap[$name]];
	}
}

function set_db_param($con, $name, &$param, $dflt = false)
{
	set_param($name, $param, $dflt);
	$param = mysqli_real_escape_string($con, $param);
}

set_param('func', $func, false);
set_param('xsl', $xsl, false);
set_param('base', $base, false);
set_param('format', $format, 'normal');

function get_xsl()
{
	global $xsl;
	if($xsl)
		return "\n<?xml-stylesheet type=\"text/xsl\" href=\"xsl/$xsl.xsl\"?>\n\n";
	return '';
}

if($base && $base != 'dummy')
{
	@$con = mysqli_connect('176.56.235.126', 'oadb', '', $base);
	if(mysqli_connect_errno($con))
		error(mysqli_connect_error());
}

if($func == "get_bases")
{
	$bases = array
	(
		'oadb' => 'ZimsNEW Instantgib'
		, 'oadb_aw' => ' ZimsNEW Allweapons'
		, 'oadb3' => 'ZimsMall Instantgib'
	);
	
	$tag = $format == 'generic' ? 'item' : 'year';
	$tags = $tag . 's';
	$key = $format == 'generic' ? 'key' : 'id';
	$value = $format == 'generic' ? 'value' : 'name';
	
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo get_xsl();
	echo "<$tags>\n";
	
	foreach($bases as $id => $name)
	{
		echo "\t<$tag>\n";
		echo "\t\t<$key>$id</$key>\n";
		echo "\t\t<$value>$name</$value>\n";
		echo "\t</$tag>\n";
	}
	echo "</$tags>\n";
}
else if($func == 'get_years')
{
	@$result = mysqli_query($con, 'select distinct YEAR(`date`) from `game`');
	if(!$result)
		error(mysqli_error($con));
	
	$tag = $format == 'generic' ? 'item' : 'year';
	$tags = $tag . 's';
	
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo get_xsl();
	echo "<$tags>\n";
	while(($row = mysqli_fetch_array($result)))
		echo "\t<$tag>$row[0]</$tag>\n";
	echo "</$tags>\n";
	
	mysqli_free_result($result);
}
else if($func == 'get_months')
{
	set_db_param($con, 'year', $year);
	
	if($year)
		$where = "where YEAR(`date`) = '$year'";
	
	@$result = mysqli_query($con, "select distinct MONTH(`date`),MONTHNAME(`date`) from `game` $where order by MONTH(`date`)");
	if(!$result)
		error(mysqli_error($con));
	
	$tag = $format == 'generic' ? 'item' : 'month';
	$tags = $tag . 's';
	$key = $format == 'generic' ? 'key' : 'number';
	$value = $format == 'generic' ? 'value' : 'name';
		
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo get_xsl();
	echo "<$tags>\n";
	while(($row = mysqli_fetch_array($result)))
	{
		echo "\t<$tag>\n";
		echo "\t<$key>$row[0]</$key>\n";
		echo "\t<$value>$row[1]</$value>\n";
		echo "\t</$tag>\n";
	}
	echo "</$tags>\n";
	
	mysqli_free_result($result);
}
else if($func == 'get_maps')
{
	set_db_param($con, 'sdate', $sdate);
	set_db_param($con, 'edate', $edate);
	
	$sep = "where";
	
	if($sdate)
		{ $where = "$sep `date` >= '$sdate'"; $sep = "and"; }
	if($edate)
		{ $where = "$where $sep `date` >= '$sdate'"; $sep = "and"; }
	
	
	@$result = mysqli_query($con, "select distinct `map` from `game` $where");
	if(!$result)
		error(mysqli_error($con));
	
	$tag = $format == 'generic' ? 'item' : 'map';
	$tags = $tag . 's';
	
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo get_xsl();
	echo "<$tags>\n";
	while(($row = mysqli_fetch_array($result)))
		echo "\t<$tag>$row[0]</$tag>\n";
	echo "</$tags>\n";
	
	mysqli_free_result($result);
}
else if($func == 'get_players')
{
	set_db_param($con, 'sdate', $sdate);
	set_db_param($con, 'edate', $edate);
	set_db_param($con, 'map', $map);
	set_db_param($con, 'game', $game);
	
	if($game)
	{
		$where = "`guid` in (select guid from kills where game_id = '$game')"
			. " and `guid` in (select guid from deaths where game_id = '$game')"
			. " and `guid` in (select guid from caps where game_id = '$game')";
	}
	else
	{
		$sep = "where";
	
		if($sdate)
			{ $subwhere = "$sep `date` >= '$sdate'"; $sep = "and"; }
		if($edate)
			{ $subwhere = "$subwhere $sep `date` >= '$sdate'"; $sep = "and"; }
		if($map)
			{ $subwhere = "$subwhere $sep `map` = '$map'"; $sep = "and"; }

			if(isset($subwhere))
				$subwhere = "where game_id in (select `game_id` from `game` $subwhere)";
			
		$where = "where `guid` in (select guid from kills $subwhere)"
			. " and `guid` in (select guid from deaths $subwhere)"
			. " and `guid` in (select guid from caps $subwhere)";
	}
	
	$sql = "select `guid`,`name` from `player` $where";
	
	//echo "<div>SQL: $sql</div>";
	
	@$result = mysqli_query($con, $sql);
	if(!$result)
		error(mysqli_error($con));
	
	$tag = $format == 'generic' ? 'item' : 'player';
	$tags = $tag . 's';
	$key = $format == 'generic' ? 'key' : 'guid';
	$value = $format == 'generic' ? 'value' : 'name';
		
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo get_xsl();
	echo "<$tags>\n";
	while(($row = mysqli_fetch_array($result)))
	{
		$safe = "";
		for($i = 0; i < strlen($row[1]); ++$i)
			if(ctype_print($row[1][$i]))
				$safe = $safe . $row[1][$i];
		$html = htmlentities($safe);
		$cdata = "<![CDATA[$html]]]>";
		echo "\t<$tag>\n";
		echo "\t\t<$key>$row[0]</$key>\n";
		echo "\t\t<$value>$cdata</$value>\n";
		echo "\t</$tag>\n";
	}
	echo "</$tags>\n";
	
	mysqli_free_result($result);
}
else if($func == 'get_polls')
{
	@$result = mysqli_query($con, 'select distinct `date` from `polls` where `type` = \'map\'');
	if(!$result)
		error(mysqli_error($con));
	
	$tag = $format == 'generic' ? 'item' : 'poll';
	$tags = $tag . 's';
	
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo get_xsl();
	echo "<$tags>\n";
	while(($row = mysqli_fetch_array($result)))
		echo "\t<$tag>$row[0]</$tag>\n";
	echo "</$tags>\n";
	
	mysqli_free_result($result);
}
else if($func == 'get_poll')
{
	set_db_param($con, 'poll', $poll);
	
	$table = array();
	
	@$result = mysqli_query($con, 'select `item`,`love`,`hate` from `polls` where `type` = \'map\' and `date` = \'' . $poll . '\'');
	if(!$result)
		error(mysqli_error($con));
	
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo get_xsl();
	echo "<poll>\n";
	for($i = 0; ($row = mysqli_fetch_array($result)); ++$i)
	{
		echo "\t<map>\n";
		echo "\t\t<name>$row[0]</name>\n";
		echo "\t\t<love>$row[1]</love>\n";
		echo "\t\t<hate>$row[2]</hate>\n";
		echo "\t</map>\n";
	}
	echo "</poll>\n";
	
	mysqli_free_result($result);
}
else if($func == 'get_stats')
{
}
else
{
	error("Unknown function: $func");
}
