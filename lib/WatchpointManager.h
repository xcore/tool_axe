#ifndef _WatchpointManager_h_
#define _WatchpointManager_h_

#include <cassert>
#include <set>
#include <stdint.h>

namespace axe {

enum class WatchpointType {
  UNSET,
  READ,
  WRITE
};

struct Watchpoint {
  WatchpointType type;
  uint32_t begin;
  uint32_t end;

  Watchpoint(WatchpointType type, uint32_t begin, uint32_t end) :
    type(type), begin(begin), end(end) {}

  bool operator<(const Watchpoint &other) const {
    if (type != other.type)
      return type < other.type;
    if (begin != other.begin)
      return begin < other.begin;
    return end < other.end;
  }

  bool operator==(const Watchpoint &other) const {
    return type == other.type &&
           begin == other.begin &&
           end == other.end;
  }
};

class WatchpointManager {
private:
  std::set<Watchpoint> watchpoints;
  std::set<Watchpoint>::iterator watchpointsIterator;
public:
  void setWatchpoint(WatchpointType type, uint32_t lowAddr, uint32_t highAddr);
  void unsetWatchpoint(WatchpointType type, uint32_t lowAddr, uint32_t highAddr);
  void clearWatchpoints() { watchpoints.clear(); }
  bool empty() const { return watchpoints.empty(); }

  bool isWatchpointAddress(WatchpointType t, uint32_t address, uint8_t ldst_size);
};

}; // End axe namespace

#endif //_WatchpointManager_h_
