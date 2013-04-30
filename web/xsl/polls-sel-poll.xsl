<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	
<xsl:param name="id" select="'xsl-polls-sel-poll'"/>
<xsl:param name="prefix" select="$id"/>
	
<xsl:template match="/">
	<select>
		<xsl:attribute name="id">
			<xsl:value-of select='$id'/>
		</xsl:attribute>
		<xsl:attribute name="class">
			<xsl:value-of select='$prefix'/>
		</xsl:attribute>
		<xsl:for-each select='polls/poll'>
			<xsl:sort select='.' order='ascending'/>
				<option><xsl:value-of select='.'/></option>
		</xsl:for-each>
	</select>
</xsl:template>

</xsl:stylesheet> 