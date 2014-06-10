// Copyright (c) 2014, XMOS Ltd., All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _TrapInfo_h_
#define _TrapInfo_h_

#include <string>

namespace axe {

class LoadedElf;

bool
getDescriptionFromTrapInfoSection(const LoadedElf *loadedElf, uint32_t spc,
                                  std::string &desc);

} // End axe namespace

#endif // _TrapInfo_h_
