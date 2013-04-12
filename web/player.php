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
<?php
if($form_map)
{
	// page here
}
?>
</body>
<?php ?>
mysqli_close($con);
?>
