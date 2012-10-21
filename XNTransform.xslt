<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns="http://www.xmos.com" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  <xsl:template match="Core">
    <Tile>
      <xsl:apply-templates select="@*|node()"/>
    </Tile>
  </xsl:template>
  <xsl:template match="@Core">
    <xsl:attribute name="Tile" select="."/>
  </xsl:template>
</xsl:stylesheet>
