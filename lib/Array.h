// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Array_h_
#define _Array_h_

namespace axe {

template <typename T, unsigned size>
constexpr unsigned arraySize(T (&)[size]) {
  return size;
}

};

#endif // _Array_h_
