#include "WatchpointManager.h"
#include "Tracer.h"
#include <set>

using namespace axe;

void WatchpointManager::setWatchpoint(WatchpointType type, uint32_t lowAddr, uint32_t highAddr)
{
  Watchpoint w = Watchpoint(type, lowAddr, highAddr);
  watchpoints.insert(w);
}

void WatchpointManager::unsetWatchpoint(WatchpointType type, uint32_t lowAddr, uint32_t highAddr)
{
  watchpoints.erase(Watchpoint(type, lowAddr, highAddr));
}

bool WatchpointManager::contains(Watchpoint w)
{
  watchpointsIterator = watchpoints.find(w);
  return !(watchpointsIterator == watchpoints.end());
}

bool WatchpointManager::isWatchpointAddress(WatchpointType t, uint32_t address)
{
  watchpointsIterator = watchpoints.begin();
  while(watchpointsIterator != watchpoints.end())
  {
    if((*watchpointsIterator).type == t &&					// Check same type
            (*watchpointsIterator).begin <= address &&	// Check addr >= lower bound
            (*watchpointsIterator).end >= address)	// Check addr <= upper bound
            return true;
    std::advance(watchpointsIterator, 1);
  }
  return false;
}
