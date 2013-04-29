<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	
<xsl:template match="/">
	<xsl:for-each select='bases/base'>
		<xsl:sort select='love div (love + hate)' order='ascending'/>
		<option>
			<xsl:attribute name="value">
				<xsl:value-of select="id" />
			</xsl:attribute>
			<xsl:if test="@sel = '1'">
				<xsl:attribute name="selected">
				</xsl:attribute>
			</xsl:if> 
			<xsl:value-of select='name'/>
		</option>
	</xsl:for-each>
</xsl:template>

</xsl:stylesheet> 