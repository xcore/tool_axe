// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _ScopedArray_h_
#define _ScopedArray_h_

#include <cstddef>

namespace axe {

template<class T> class scoped_array
{
private:
  T *ptr;
  scoped_array(scoped_array const &);
  scoped_array &operator=(scoped_array const &);
public:
  scoped_array(T *p) : ptr(p) {}

  ~scoped_array() {
    delete[] ptr;
  }

  T &operator[](std::ptrdiff_t i) const
  {
    return ptr[i];
  }

  T *get() const
  {
    return ptr;
  }
};
  
} // End axe namespace

#endif // _ScopedArray_h_
