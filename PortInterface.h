// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortInterface_h_
#define _PortInterface_h_

#include "Config.h"

struct Signal;

class PortInterface {
public:
  virtual void seePinsChange(const Signal &value, ticks_t time) = 0;
};

/// Port interface which delgates seePinsChange calls to a member function of
/// another class.
template <class T>
class PortInterfaceMemberFuncDelegate : public PortInterface {
  T &obj;
  void (T::*func)(const Signal &value, ticks_t time);
public:
  PortInterfaceMemberFuncDelegate(T &o, void (T::*f)(const Signal &value, ticks_t time)) :
  obj(o),
  func(f) {}
  void seePinsChange(const Signal &value, ticks_t time) {
    (obj.*func)(value, time);
  }
};

#endif // _PortInterface_h_
