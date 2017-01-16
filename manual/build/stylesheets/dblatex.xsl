<?xml version='1.0' encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'>

  <xsl:param name="latex.encoding">utf8</xsl:param>
  <xsl:param name="latex.class.options">10pt,onecolumn</xsl:param>
  <xsl:param name="xetex.font">
   <xsl:choose>
    <xsl:when test="contains(/book/@lang,'ja')">
	<xsl:text>\usepackage{xeCJK}&#10;</xsl:text>
	<xsl:text>\setCJKmainfont{VL-PGothic-Regular}&#10;</xsl:text>
	<xsl:text>\setCJKsansfont{VL-PGothic-Regular}&#10;</xsl:text>
	<xsl:text>\setCJKmonofont{VL-PGothic-Regular}&#10;</xsl:text>
        <xsl:text>\setmainfont{VL-PGothic-Regular}&#10;</xsl:text>
        <xsl:text>\setsansfont{VL-PGothic-Regular}&#10;</xsl:text>
        <xsl:text>\setmonofont{VL-PGothic-Regular}&#10;</xsl:text>
    </xsl:when>
    <xsl:when test="contains(/book/@lang,'ko')">
	<xsl:text>\usepackage{xeCJK}&#10;</xsl:text>
	<xsl:text>\setCJKmainfont{WenQuanYi Micro Hei}&#10;</xsl:text>
	<xsl:text>\setCJKsansfont{WenQuanYi Micro Hei}&#10;</xsl:text>
	<xsl:text>\setCJKmonofont{WenQuanYi Micro Hei Mono}&#10;</xsl:text>
	<xsl:text>\setmainfont{WenQuanYi Micro Hei}&#10;</xsl:text>
	<xsl:text>\setsansfont{WenQuanYi Micro Hei}&#10;</xsl:text>
	<xsl:text>\setmonofont{WenQuanYi Micro Hei Mono}&#10;</xsl:text>
    </xsl:when>
    <xsl:when test="contains(/book/@lang,'zh')">
	<xsl:text>\usepackage{xeCJK}&#10;</xsl:text>
	<xsl:text>\setCJKmainfont{WenQuanYi Micro Hei}&#10;</xsl:text>
	<xsl:text>\setCJKsansfont{WenQuanYi Micro Hei}&#10;</xsl:text>
	<xsl:text>\setCJKmonofont{WenQuanYi Micro Hei Mono}&#10;</xsl:text>
	<xsl:text>\setmainfont{WenQuanYi Micro Hei}&#10;</xsl:text>
	<xsl:text>\setsansfont{WenQuanYi Micro Hei}&#10;</xsl:text>
	<xsl:text>\setmonofont{WenQuanYi Micro Hei Mono}&#10;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
        <xsl:text>\setmainfont{FreeSerif}&#10;</xsl:text>
        <xsl:text>\setsansfont{FreeSans}&#10;</xsl:text>
        <xsl:text>\setmonofont{FreeMono}&#10;</xsl:text>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:param>
</xsl:stylesheet>
