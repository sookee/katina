<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	
<xsl:template match="/">
<html>
<head>
<link rel="stylesheet" type="text/css" href="poll.css"/>
</head>
<body>
<h1>Votes</h1>
<form id="votes-form" action="katina.php" method="get">
	
</form>
<table border='1'>
	<tr bgcolor='#9acd32'>
		<th>Map</th>
		<th>Love</th>
		<th>Hate</th>
		<th>Votes</th>
		<th>Like</th>
	</tr>
	<xsl:for-each select='poll/map'>
		<xsl:sort select='love div (love + hate)' order='descending'/>
		<tr>
			<td><xsl:value-of select='name'/></td>
			<td><xsl:value-of select='love'/></td>
			<td><xsl:value-of select='hate'/></td>
			<td><xsl:value-of select='love + hate'/></td>
			<td><xsl:value-of select='format-number((love * 100) div (love + hate), "#.00")'/>%</td>
		</tr>
	</xsl:for-each>
</table>
</body>
</html>
</xsl:template>

</xsl:stylesheet> 