// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _AccessSecondIterator_h_
#define _AccessSecondIterator_h_

#include <boost/iterator/transform_iterator.hpp>
#include <iterator>

namespace axe {

template <typename Iterator>
struct ValueGetter
{
  const typename Iterator::value_type::second_type &
  operator() (const typename Iterator::value_type& p) const {
    return p.second;
  }
};

template <typename Iterator> using AccessSecondIterator =
boost::transform_iterator<
ValueGetter<Iterator>, Iterator>;

} // End axe namespace

#endif // _AccessSecondIterator_h_
