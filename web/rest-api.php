<?php
header("Content-type: text/xml");
date_default_timezone_set('GMT');

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

function get_xsl()
{
	global $xsl;
	if($xsl)
		return "\n<?xml-stylesheet type=\"text/xsl\" href=\"$xsl.xsl\"?>\n\n";
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
	
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo get_xsl();
	echo "<bases>\n";
	
	foreach($bases as $id => $name)
	{
		$sel = $id == $base;
		echo "\t<base sel='$sel'>\n";
		echo "\t\t<id>$id</id>\n";
		echo "\t\t<name>$name</name>\n";
		echo "\t</base>\n";
	}
	echo "</bases>\n";
}
else if($func == 'get_polls')
{
	set_db_param($con, 'poll', $poll);
	
	@$result = mysqli_query($con, 'select distinct `date` from `polls` where `type` = \'map\'');
	if(!$result)
		error(mysqli_error($con));
	
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	echo get_xsl();
	echo "<polls>\n";
	while(($row = mysqli_fetch_array($result)))
	{
		$sel = $row[0] == $poll;
		echo "\t<poll sel='$sel'>$row[0]</poll>\n";
	}
	echo "</polls>\n";
	
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
else
{
	error("Unknown function: $func");
}
