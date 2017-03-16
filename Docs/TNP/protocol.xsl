<?xml version='1.0' ?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">	
  <xsl:template match="/">
    <html>
      <head>
        <title><xsl:value-of select="/protocol/@name" /></title>
        
        <style type="text/css">
          * {
            margin:0px;
            padding:0px;
          }
          
          body {
            padding:10px;
          }
          
          li.collapsed ul {
            display:none;
          }
          
          ul.tree li {
            position:relative;
            padding:5px;
            padding-left:15px;
          }
          
          ul.tree {
            margin:5px;
          }
          
          ul.tree ul {
            margin:5px;
          }
          
          li h3 {
            cursor:pointer;
          }

          body {
            font-family:verdana, arial, sans-serif;
            font-size:12px;
          }
          
          li.field, li.unit {
            margin-bottom:5px;
          }
          
          li.unit {
            border:dotted 1px #A0A0A0;
            padding-left:5px;
          }
          
          span.constant {
            font-style:italic;
          }
          
          span.type {
            color:#A0A0A0;
            margin-right:5px;
            position:relative;
            padding:1px;
          }
          
          span.type:hover {
            background-color:#A0A0A0;
            color:white;
          }
          
          span.type div.info {
            position:absolute;
            top:15px;
            display:none;
            border:solid 1px #A0A0A0;
            background-color:#EFEFEF;
            padding:10px;
            z-index:10;
            font-weight:normal;
            font-size:10px;
            min-width:400px;
            color:black;
          }
          
          span.type:hover div.info {
            display:block;
          }
          
          ul.tree h3 {
            font-weight:bold;
            font-size:12px;
            display:inline;
          }
          
          ul.tree span.name {
            text-decoration:underline;
          }
          
          span.size {
            float:left;
            border:solid 1px #A0A0A0;
            padding:1px;
            background-color:#EFEFEF;
            margin-right:3px;
            display:block;
            vertical-align:middle;
            position:absolute;
            left:2px;
            top:2px;
          }
          
          span.range {
            color:#A0A0A0;
          }
          
          li.option {
            border:dotted 1px blue;
            padding-left:5px;
            margin-bottom:5px;
          }
          
          abbr {
            border-bottom:dotted 1px #A0A0A0;
          }
          
        </style>
        
        <script type="text/javascript">
          <![CDATA[
          function hasClass(ele,cls) {
            return ele.className.match(new RegExp('(\\s|^)'+cls+'(\\s|$)'));
          }
          function addClass(ele,cls) {
            if (!this.hasClass(ele,cls)) ele.className += " "+cls;
          }
          function removeClass(ele,cls) {
            if (hasClass(ele,cls)) {
              var reg = new RegExp('(\\s|^)'+cls+'(\\s|$)');
              ele.className=ele.className.replace(reg,' ');
            }
          }


          function makeTree(ul) {
            var children = ul.childNodes;
            for(var a=0;a<children.length;a++) {
              var node = children[a];
              if(node.tagName=="LI") {
                var oldClassName = node.className;
                node.className = node.className + " collapsed";
                node.onclick = function(e) { if(hasClass(this, 'collapsed')) { removeClass(this,'collapsed'); } else { addClass(this,'collapsed'); } e.stopPropagation(); return false; }
              }
              makeTree(node);
            }
          }
          ]]>
        </script>
      </head>
      
      <body onload="makeTree(document.getElementById('tree'))">
        <h1><xsl:value-of select="/protocol/@name" /> v<xsl:value-of select="/protocol/@version" /></h1>
        <br/>
        
        <h2>Protocol units</h2>
        <ul class="tree" id="tree">
          <xsl:apply-templates select="/protocol/units/unit" />
        </ul>
      </body>
    </html>
  </xsl:template>
  
  <xsl:template match="unit">
    <li class="unit">
      <h3><span class="type"><xsl:value-of select="@type" /></span> <span class="name"><xsl:value-of select="@name" /></span></h3>
      <ul class="tree">
        <xsl:apply-templates />
      </ul>
    </li>
  </xsl:template>
  
  <xsl:template match="@type">
    <xsl:variable name="name" select="." />
    <xsl:variable name="type" select="//types/type[@name=$name]" />
        
    <span class="type">
      <xsl:value-of select="." />
      <xsl:if test="$type">
        <div class="info">
          <xsl:if test="$type/@size">
            Size: <xsl:value-of select="$type/@size" /> bytes<br/>
          </xsl:if>
          <xsl:apply-templates select="$type/value" /><br/>
          <xsl:value-of select="$type" />
        </div>
      </xsl:if>
    </span>
  </xsl:template>
  
  <xsl:template match="field">
    <li class="field">
      <xsl:if test="@size">
        <span class="size"><xsl:value-of select="@size"/></span>
      </xsl:if>
      <h3><xsl:apply-templates select="@type" /> <span class="name"><xsl:value-of select="@name" /></span></h3>
      <!-- Field with fixed value set -->
      <xsl:if test="count(value) &gt; 0">
        = 
        <xsl:apply-templates select="value" />
      </xsl:if>
      <br/>
      <em><xsl:value-of select="." /></em>
    </li>
  </xsl:template>
  
  <xsl:template match="value">
    <span>
      <xsl:attribute name="class">
        value
        <xsl:if test="@id"> constant</xsl:if>
      </xsl:attribute>

      <abbr>
        <xsl:attribute name="title">
          <xsl:value-of select="@id" />
        </xsl:attribute>
        
        <xsl:value-of select="@name" />
        <xsl:if test="@from and @to">
          <span class="range">
          [<xsl:value-of select="@from"/>,<xsl:value-of select="@to" />]
          </span>
        </xsl:if>
      </abbr>
    </span> 
    <xsl:if test="position()!=last()"> | </xsl:if>
  </xsl:template>
  
  <xsl:template match="choice">
      Either of the following:
      <ul>
        <xsl:apply-templates select="option" />
      </ul>
    
  </xsl:template>
  
  <xsl:template match="option">
    <li class="option">
      <xsl:if test="count(when)&gt;0">
        <h3>When </h3> <span class="name"><xsl:apply-templates select="when" /></span>
      </xsl:if>
      
      <ul>
        <xsl:apply-templates select="*[not(local-name()='when')]" />
      </ul>
    </li>
  </xsl:template>
  
  <xsl:template match="when">
      <xsl:value-of select="@in-unit" />-&gt;<xsl:value-of select="@field" />
      <xsl:apply-templates />
      <xsl:if test="position()!=last()"> and </xsl:if>
  </xsl:template>
  
  <xsl:template match="when/value">
    == <span class="constant value"><xsl:value-of select="@name" /></span>
  </xsl:template>
</xsl:stylesheet>
