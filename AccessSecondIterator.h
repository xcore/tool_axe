// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _AccessSecondIterator_h_
#define _AccessSecondIterator_h_

#include <iterator>

namespace axe {

template <typename Iterator>
class AccessSecondIterator :
public std::iterator<
       std::forward_iterator_tag,
       typename std::iterator_traits<Iterator>::value_type::second_type> {
  Iterator it;
public:
  AccessSecondIterator() {}
  AccessSecondIterator(Iterator i) : it(i) {}
  AccessSecondIterator &operator++() {
    ++it;
    return *this;
  }
  AccessSecondIterator operator++(int) {
    AccessSecondIterator tmp(*this);
    operator++();
    return tmp;
  }
  const typename std::iterator_traits<Iterator>::value_type::second_type &
  operator*() const {
    return it->second;
  }
  const typename std::iterator_traits<Iterator>::value_type::second_type *
  operator->() const {
    return &**this;
  }
  bool operator==(const AccessSecondIterator &other) {
    return it == other.it;
  }
  bool operator!=(const AccessSecondIterator &other) {
    return !(*this == other);
  }
};

} // End axe namespace

#endif // _AccessSecondIterator_h_
