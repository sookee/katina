<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	
<xsl:param name="id" select="'xsl-rest-sel-base'"/>
<xsl:param name="prefix" select="$id"/>

<xsl:template match="/">
	<select id="{$id}" class="{$prefix}-sel">
		<xsl:for-each select="items/item">
			<option class="{$prefix}-opt">
				<xsl:attribute name="value">
					<xsl:value-of select="key"/>
				</xsl:attribute>
				<xsl:value-of select="value"/>
			</option>
		</xsl:for-each>		
	</select>

</xsl:template>

</xsl:stylesheet> 