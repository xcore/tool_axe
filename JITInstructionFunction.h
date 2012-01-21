// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _JITInstructionFunction_h_
#define _JITInstructionFunction_h_

class Thread;

typedef bool (*JITInstructionFunction_t)(Thread &);

#endif // _JITInstructionFunction_h_
