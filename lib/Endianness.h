// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Endianness_h_
#define _Endianness_h_

#include <boost/detail/endian.hpp>

#if defined(BOOST_LITTLE_ENDIAN)
#define HOST_LITTLE_ENDIAN 1
#elif defined(BOOST_BIG_ENDIAN)
#define HOST_LITTLE_ENDIAN 0
#else
#error "Unknown endianness"
#endif

#endif // _Endianness_h_
