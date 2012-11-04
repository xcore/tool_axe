// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _XMLUtils_h_
#define _XMLUtils_h_

#include <libxml/parser.h>
#include <libxml/tree.h>

namespace axe {
  
xmlNode *findChild(xmlNode *node, const char *name);
xmlAttr *findAttribute(xmlNode *node, const char *name);
xmlDoc *applyXSLTTransform(xmlDoc *doc, const char *transformData);
bool checkDocAgainstSchema(xmlDoc *doc, const char *schemaData,
                           size_t schemaSize);

} // End axe namespace

#endif // _XMLUtils_h_
