<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:param name="id" select="'xsl-polls-table'"/>
<xsl:param name="prefix" select="$id"/>
	
<xsl:template match="/">
	<xsl:variable name="pcf" select="'#%'"/>
	<xsl:variable name="head" select="concat($prefix, '-thead')"/>
	<xsl:variable name="body" select="concat($prefix, '-tbody')"/>
	<xsl:variable name="foot" select="concat($prefix, '-tfoot')"/>
	<table id="{$id}" class="{$prefix}-table">
		<thead>
			<tr class="{$head}-tr">
				<th class="{$head}-th {$head}-map">Map</th>
				<th class="{$head}-th {$head}-love">Love</th>
				<th class="{$head}-th {$head}-hate">Hate</th>
				<th class="{$head}-th {$head}-feel">Feel</th>
				<th class="{$head}-th {$head}-votes">Votes</th>
				<th class="{$head}-th {$head}-like">Like</th>
			</tr>
		</thead>
		<tbody>
			<xsl:for-each select='poll/map'>
				<xsl:sort select='love div (love + hate)' order='descending'/>
				<tr class="{$body}-tr">
					<td class="{$body}-td {$body}-map"><xsl:value-of select='name'/></td>
					<td class="{$body}-td {$body}-love"><xsl:value-of select='love'/></td>
					<td class="{$body}-td {$body}-hate"><xsl:value-of select='hate'/></td>
					<td class="{$body}-td {$body}-feel"><xsl:value-of select='love - hate'/></td>
					<td class="{$body}-td {$body}-votes"><xsl:value-of select='love + hate'/></td>
					<td class="{$body}-td {$body}-like"><xsl:value-of select='format-number(love div (love + hate), $pcf)'/></td>
				</tr>
			</xsl:for-each>
		</tbody>
		<tfoot>
			<tr class="{$foot}-tr">
				<td class="{$foot}-td {$foot}-map">Total</td>
				<td class="{$foot}-td {$foot}-love"><xsl:value-of select='sum(poll/map/love)'/></td>
				<td class="{$foot}-td {$foot}-hate"><xsl:value-of select='sum(poll/map/hate)'/></td>
				<td class="{$foot}-td {$foot}-feel"><xsl:value-of select='sum(poll/map/love) - sum(poll/map/hate)'/></td>
				<td class="{$foot}-td {$foot}-votes"><xsl:value-of select='sum(poll/map/love) + sum(poll/map/hate)'/></td>
				<td class="{$foot}-td {$foot}-like"><xsl:value-of select='format-number(sum(poll/map/love) div (sum(poll/map/love) + sum(poll/map/hate)), $pcf)'/></td>
			</tr>
		</tfoot>	
	</table>
</xsl:template>

</xsl:stylesheet> 