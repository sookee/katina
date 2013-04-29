<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	
<xsl:template match="/">
	<xsl:variable name="pcformat" select="'#%'"/>
	<table border='1'>
		<thead>
			<tr bgcolor='#9acd32'>
				<th>Map</th>
				<th>Love</th>
				<th>Hate</th>
				<th>Feel</th>
				<th>Votes</th>
				<th>Like</th>
			</tr>
		</thead>
		<tbody>
			<xsl:for-each select='poll/map'>
				<xsl:sort select='love div (love + hate)' order='descending'/>
				<tr>
					<td><xsl:value-of select='name'/></td>
					<td><xsl:value-of select='love'/></td>
					<td><xsl:value-of select='hate'/></td>
					<td><xsl:value-of select='love - hate'/></td>
					<td><xsl:value-of select='love + hate'/></td>
					<td><xsl:value-of select='format-number(love div (love + hate), $pcformat)'/></td>
				</tr>
			</xsl:for-each>
		</tbody>
		<tfoot>
			<tr bgcolor='#cd9a32'>
				<td>Total</td>
				<td><xsl:value-of select='sum(poll/map/love)'/></td>
				<td><xsl:value-of select='sum(poll/map/hate)'/></td>
				<td><xsl:value-of select='sum(poll/map/love) - sum(poll/map/hate)'/></td>
				<td><xsl:value-of select='sum(poll/map/love) + sum(poll/map/hate)'/></td>
				<td><xsl:value-of select='format-number(sum(poll/map/love) div (sum(poll/map/love) + sum(poll/map/hate)), $pcformat)'/></td>
			</tr>
		</tfoot>	
	</table>
</xsl:template>

</xsl:stylesheet> 