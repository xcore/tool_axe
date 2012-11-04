// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "XMLUtils.h"
#include <libxml/relaxng.h>
#include <libxslt/transform.h>
#include <cstring>

xmlNode *axe::findChild(xmlNode *node, const char *name)
{
  for (xmlNode *child = node->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE)
      continue;
    if (std::strcmp(name, (char*)child->name) == 0)
      return child;
  }
  return 0;
}

xmlAttr *axe::findAttribute(xmlNode *node, const char *name)
{
  for (xmlAttr *child = node->properties; child; child = child->next) {
    if (child->type != XML_ATTRIBUTE_NODE)
      continue;
    if (std::strcmp(name, (char*)child->name) == 0)
      return child;
  }
  return 0;
}

xmlDoc *axe::
applyXSLTTransform(xmlDoc *doc, const char *transformData)
{
  xmlDoc *transformDoc =
  xmlReadDoc(BAD_CAST transformData, "XNTransform.xslt", NULL, 0);
  xsltStylesheetPtr stylesheet = xsltParseStylesheetDoc(transformDoc);
  xmlDoc *newDoc = xsltApplyStylesheet(stylesheet, doc, NULL);
  xsltFreeStylesheet(stylesheet);
  return newDoc;
}

bool axe::
checkDocAgainstSchema(xmlDoc *doc, const char *schemaData, size_t schemaSize)
{
  xmlRelaxNGParserCtxtPtr schemaContext =
  xmlRelaxNGNewMemParserCtxt(schemaData, schemaSize);
  xmlRelaxNGPtr schema = xmlRelaxNGParse(schemaContext);
  xmlRelaxNGValidCtxtPtr validationContext =
  xmlRelaxNGNewValidCtxt(schema);
  bool isValid = xmlRelaxNGValidateDoc(validationContext, doc) == 0;
  xmlRelaxNGFreeValidCtxt(validationContext);
  xmlRelaxNGFree(schema);
  xmlRelaxNGFreeParserCtxt(schemaContext);
  return isValid;
}
