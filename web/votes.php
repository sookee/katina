<?php
session_start();

date_default_timezone_set('GMT');

$con = mysqli_connect("176.56.235.126", "oadb", "", "oadb");
if(mysqli_connect_errno($con))
	echo mysqli_connect_error();


// `type` varchar(8) NOT NULL,
// `item` varchar(32) NOT NULL,
// `guid` varchar(8) NOT NULL,
// `count` int(4) NOT NULL,

$sql = 'select `item`,`guid`,`count` from `votes` where `type` = \'map\'';
$result = mysqli_query($con, $sql);
if(!$result)
	die(mysqli_error($con));

//  map | love | hate | total | %age

$table = array(); // map => array();

while($row = mysqli_fetch_array($result))
{
	if(!isset($table[$row[0]]))
		$table[$row[0]] = array();
	if(!isset($table[$row[0]]['love']))
		$table[$row[0]]['love'] = 0;
	if(!isset($table[$row[0]]['love']))
		$table[$row[0]]['hate'] = 0;
	if(!isset($table[$row[0]]['love']))
		$table[$row[0]]['total'] = 0;
	if(!isset($table[$row[0]]['love']))
		$table[$row[0]]['%age'] = 0;

	if($row[2] < 0)
		++$table[$row[0]]['hate'];
	else if($row[2] > 0)
		++$table[$row[0]]['love'];
}

mysqli_free_result($result);
?>
