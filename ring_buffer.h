// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _ring_buffer_h_
#define _ring_buffer_h_

namespace axe {

template <typename Kind, unsigned BufSize>
class ring_buffer {
private:
  Kind buf[BufSize];
  unsigned writeIdx;
  unsigned readIdx;
  unsigned numEntries;
public:
  ring_buffer()
    : writeIdx(0),
      readIdx(0),
      numEntries(0) {}
  
  bool empty() const
  {
    return numEntries == 0;
  }
  
  bool full() const
  {
    return numEntries == BufSize;
  }
  
  void push_back(const Kind &t)
  {
    buf[writeIdx] = t;
    writeIdx = (writeIdx + 1) % BufSize;
    numEntries++;
  }
  
  void pop_front()
  {
    readIdx = (readIdx + 1) % BufSize;
    numEntries--;
  }

  void pop_front(unsigned num)
  {
    readIdx = (readIdx + num) % BufSize;
    numEntries-=num;
  }
  
  unsigned size() const
  {
    return numEntries;
  }

  unsigned capacity() const
  {
    return BufSize;
  }

  unsigned remaining() const
  {
    return capacity() - size();
  }
  
  Kind &front()
  {
    return buf[readIdx];
  }
  
  const Kind &front() const
  {
    return buf[readIdx];
  }
  
  Kind &back()
  {
    return *const_cast<Kind *>(
      &static_cast<const ring_buffer<Kind, BufSize>*>(this)->back());
  }

  const Kind &back() const
  {
    return buf[(writeIdx - 1) % BufSize];
  }
  
  Kind &operator[](unsigned idx)
  {
    return *const_cast<Kind *>(
      &(*static_cast<const ring_buffer<Kind, BufSize>*>(this))[idx]);
  }

  const Kind &operator[](unsigned idx) const
  {
    return buf[(readIdx + idx) % BufSize];
  }

  void clear()
  {
    readIdx = writeIdx;
    numEntries = 0;
  }
};
  
} // End axe namespace

#endif // _ring_buffer_h_
