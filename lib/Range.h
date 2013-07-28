// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Range_h_
#define _Range_h_

namespace axe {

  template <typename T> class range {
    T beginIt;
    T endIt;
  public:
    range(T b, T e) : beginIt(b), endIt(e) {}

    T begin() { return beginIt; }
    T end() { return endIt; }
  };

  template <typename T> range<T> make_range(T begin, T end) {
    return range<T>(begin, end);
  }

  template <typename T> range<T> make_range(const std::pair<T, T> &p) {
    return range<T>(p.first, p.second);
  }
} // End namespace axe

#endif // _Range_h_
