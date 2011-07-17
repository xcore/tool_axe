// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Signal_h_
#define _Signal_h_

#include <cassert>
#include <iterator>

struct Edge {
  enum Type {
    RISING,
    FALLING
  };
  ticks_t time;
  Type type;
  Edge() {}
  Edge(ticks_t t, Type ty) : time(t), type(ty) {}

  static Type flip(Type type) {
    return type == RISING ? FALLING : RISING;
  }
};

struct Signal;

class EdgeIterator : public std::iterator<std::random_access_iterator_tag, Edge,
                                          ticks_t> {
  Edge edge;
  uint32_t halfPeriod;
public:
  EdgeIterator() {}
  EdgeIterator(Edge e, uint32_t hp) :
    edge(e), halfPeriod(hp) {}
  EdgeIterator &operator++() {
    edge.type = Edge::flip(edge.type);
    edge.time += halfPeriod;
    return *this;
  }
  EdgeIterator operator++(int) {
    EdgeIterator tmp(*this);
    operator++();
    return tmp;
  }
  EdgeIterator &operator+=(ticks_t n) {
    if (n & 1) {
      edge.type = Edge::flip(edge.type);
    }
    edge.time += halfPeriod * n;
    return *this;
  }
  EdgeIterator operator+(ticks_t n) {
    EdgeIterator tmp(*this);
    return tmp += n;
  }
  EdgeIterator &operator--() {
    edge.type = Edge::flip(edge.type);
    edge.time -= halfPeriod;
    return *this;
  }
  EdgeIterator operator--(int) {
    EdgeIterator tmp(*this);
    operator--();
    return tmp;
  }
  EdgeIterator &operator-=(ticks_t n) {
    if (n & 1) {
      edge.type = Edge::flip(edge.type);
    }
    edge.time -= halfPeriod * n;
    return *this;
  }
  EdgeIterator operator-(ticks_t n) {
    EdgeIterator tmp(*this);
    return tmp -= n;
  }
  ticks_t operator-(const EdgeIterator &other) {
    return (edge.time - other.edge.time) / halfPeriod;
  }
  const Edge &operator*() const {
    return edge;
  }
  const Edge *operator->() const {
    return &**this;
  }
  bool operator==(const EdgeIterator &other) {
    return edge.time == other.edge.time;
  }
  bool operator!=(const EdgeIterator &other) {
    return !(*this == other);
  }
  const Edge &operator*() { return edge; }
};

/// Describes either a fixed frequency 1 bit clock signal or a constant signal
/// of a width up to 32 bits.
struct Signal {
  /// If halfPeriod is set to 0 the signal is constant where the value is given
  /// by value. If the halfPeriod is non zero the signal is a fixed frequency
  /// clock signal. In this case the signal is 0 in the interval
  /// [value, value + halfPeriod) and 1 in the interval
  /// [value + halfPeriod, value + halfPeriod * 2).
  uint32_t halfPeriod;
  uint32_t value;
  Signal() : halfPeriod(0) { value = 0; }
  explicit Signal(uint32_t v) : halfPeriod(0) { value = v; }
  Signal(ticks_t startTime, uint32_t hp) :
    halfPeriod(hp)
  {
    value = startTime % (halfPeriod * 2);
  }
  bool isClock() const { return halfPeriod != 0; }
  uint32_t getValue(ticks_t time) const {
    if (halfPeriod == 0)
      return value;
    time -= value;
    return ((time % (halfPeriod * 2)) >= halfPeriod) ? 1 : 0;
  }
  Edge getNextEdge(ticks_t time) const {
    assert(halfPeriod != 0);
    ticks_t nextEdgeNumber = 1 + (time - value) / halfPeriod;
    Edge::Type edgeType = (nextEdgeNumber & 1) ? Edge::RISING : Edge::FALLING;
    ticks_t edgeTime = value + nextEdgeNumber * halfPeriod;
    return Edge(edgeTime, edgeType);
  }
  EdgeIterator getEdgeIterator(ticks_t time) const {
    return EdgeIterator(getNextEdge(time), halfPeriod);
  }
  void changeFrequency(ticks_t time, uint32_t hp) {
    changeFrequency(time, getValue(time), hp);
  }
  void changeFrequency(ticks_t time, uint32_t initial, uint32_t hp) {
    halfPeriod = hp;
    if (halfPeriod == 0)
      value = initial;
    if (initial != 0)
      time += halfPeriod;
    value = (time % (halfPeriod * 2));      
  }
  void setFreeRunning(ticks_t time, uint32_t hp) {
    assert(hp != 0);
    halfPeriod = hp;
    value = time % (halfPeriod * 2);
  }
  void setValue(uint32_t v) {
    halfPeriod = 0;
    value = v;
  }
  uint32_t getHalfPeriod() const {
    return halfPeriod;
  }
  uint32_t getPeriod() const {
    return halfPeriod * 2;
  }
  bool operator==(const Signal &other) const
  {
    return halfPeriod == other.halfPeriod &&
           value == other.value;
  }
  bool operator!=(const Signal &other) const
  {
    return !(*this == other);
  }
};

#endif
