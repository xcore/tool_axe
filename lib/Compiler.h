// Copyright (c) 2011-2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Compiler_h_
#define _Compiler_h_

#ifdef __GNUC__
#define UNUSED(x) x __attribute__((__unused__))
#endif // __GNUC__

#ifndef UNUSED
#define UNUSED(x) x
#endif

#endif // _Compiler_h_
