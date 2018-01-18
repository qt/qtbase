<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:a="http://www.iana.org/assignments" xmlns="http://www.iana.org/assignments" xmlns:_="http://www.iana.org/assignments" xmlns:DEFAULT="http://www.iana.org/assignments" version="1.0">
<xsl:output omit-xml-declaration="yes" indent="no" method="text"/>
<xsl:template match="/a:registry[@id='cbor-tags']">struct CborTagDescription
{
    QCborTag tag;
    const char *description;    // with space and parentheses
};

// <xsl:value-of select="a:registry/a:title"/>
static const CborTagDescription tagDescriptions[] = {
    // from https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml
<xsl:for-each select="a:registry/a:record">
    <xsl:sort select="a:value" data-type="number"/>
        <xsl:if test="a:semantics != ''">
            <xsl:call-template name="row"/>
        </xsl:if>
    </xsl:for-each>    { QCborTag(-1), nullptr }
};
</xsl:template>
<xsl:template name="row">    { QCborTag(<xsl:value-of select="a:value"/>), " (<xsl:value-of select="a:semantics"/> <xsl:call-template name="xref"/>)" },
</xsl:template>
<xsl:template name="xref"><xsl:if test="a:xref/@type = 'rfc'"> [<xsl:value-of select="translate(a:xref/@data,'rfc','RFC')"/>]</xsl:if>
</xsl:template>
</xsl:stylesheet>
