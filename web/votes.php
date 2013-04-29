<?php
date_default_timezone_set('GMT');
$servermap = array
(
	'oadb' => 'ZimsNEW Instantgib'
	, 'oadb_aw' => ' ZimsNEW Allweapons'
	, 'oadb3' => 'ZimsMall Instantgib'
);

$base = isset($_POST['base']) ? $_POST['base'] :'oadb'; 
$con = mysqli_connect("176.56.235.126", "oadb", "", $base);
if(mysqli_connect_errno($con))
	echo mysqli_connect_error();

$date = isset($_POST['date']) ? mysqli_real_escape_string($con, $_POST['date']) :false; 

//echo "POST['base']: " . $_POST['base'] . "<br/>";
//echo "base: $base <br/>";

if(php_sapi_name() == 'cli')
{
	$date = "2013-04-22 09:20:15";
	$base = 'oadb';
}

/**
 * Return an array of poll dates.
 */
function get_polls($con)
{
	$polls = array();
	
	$result = mysqli_query($con, 'select distinct `date` from `polls` where `type` = \'map\'');
	if(!$result)
		die(mysqli_error($con));
	
	while(($row = mysqli_fetch_array($result)))
		$polls[$row[0]] = $row[0];
	
	mysqli_free_result($result);

	return $polls;
}

function get_poll($con, $date)
{
	$table = array();
	
	$result = mysqli_query($con, 'select `item`,`love`,`hate` from `polls` where `type` = \'map\' and `date` = \'' . $date . '\'');
	if(!$result)
		die(mysqli_error($con));
	
	for($i = 0; ($row = mysqli_fetch_array($result)); ++$i)
	{
		if(!isset($table[$i]))
			$table[$i] = array();
		
		$table[$i][] = $row[0];
		$table[$i][] = $row[1];
		$table[$i][] = $row[2];
		$table[$i][] = $row[1] + $row[2];
		$table[$i][] = sprintf('%0.2f', ($row[1] * 100) / ($row[1] + $row[2]));
	}
	
	mysqli_free_result($result);

	return $table;
}

function create_selector($name, $list, $dflt)
{
	//echo "dflt: $dflt";
	//$count = 0;
//	$simple = false;
//	if(array_values($list) === $list)
//		$simple = true;

	$html = "\n" . '<select name="' . $name . '">';
	foreach($list as $key => $item)
	{
//		if($simple)
//			$key = $item;
		echo "key: $key item: $item </br>";
		if($key == $dflt)
			$html = $html . "\n\t" . '<option value="' . $key . '" selected>' . $item . '</option>';
		else
			$html = $html . "\n\t" . '<option value="' . $key . '">' . $item . '</option>';
	}
	$html = $html . "\n" . '</select>' . "\n";
	return $html;
}
?>
<!DOCTYPE html>
<html>
	<head>
	</head>
	<body>
		<form id="vote-form" action="votes.php" method="post">
		<table id="vote-form-table">
		<tr>
		<th id="vote-form-th">
			Month
		</th>
		<th>
		</th>
		</tr>
		<tr>
			<td><?php echo create_selector('base', $servermap, $base) ?></td>
			<td><?php echo create_selector('date', get_polls($con), $date) ?></td>
			<td><input type="submit"/></td>
		</tr>
		</table>
		</form>
		<table>
			<thead>
			<tr>
				<th>Map</th>
				<th>Love</th>
				<th>Hate</th>
				<th>Votes</th>
				<th>%Like</th>
			</tr>
			</thead>
			<tbody>
			<?php
			$table = get_poll($con, $date);
			for($i = 0; $i < count($table); ++$i)
			{
			?>
			<tr>
			<?php
				foreach($table[$i] as $key => $value)
				{
			?>
				<td>
					<?php echo $value ?>
				</td>
			<?php
				}
			?>
			</tr>
			<?php
			}
			?>
			</tbody>
		</table>
	</body>
</html>
<?php
mysqli_close($con);
?>
