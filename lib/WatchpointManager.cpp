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

bool WatchpointManager::isWatchpointAddress(WatchpointType t, uint32_t address)
{
  for (const Watchpoint &watchpoint : watchpoints) {
    if (t == watchpoint.type &&
        address >= watchpoint.begin &&
        address < watchpoint.end)
      return true;
  }
  return false;
}
