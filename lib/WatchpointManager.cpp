#include "WatchpointManager.h"
#include "Tracer.h"
#include <set>

using namespace axe;

void WatchpointManager::setWatchpoint(WatchpointType type, uint8_t lowAddr, uint8_t highAddr)
{
	Watchpoint w = Watchpoint(type, std::pair<uint8_t, uint8_t>(lowAddr, highAddr));
	watchpoints.insert(w);
}

void WatchpointManager::unsetWatchpoint(WatchpointType type, uint8_t lowAddr, uint8_t highAddr)
{
	Watchpoint w = Watchpoint(type, std::pair<uint8_t, uint8_t>(lowAddr, highAddr));
}

bool WatchpointManager::contains(Watchpoint w)
{
	watchpointsIterator = watchpoints.find(w);
	return !(watchpointsIterator == watchpoints.end());
}

bool WatchpointManager::isWatchpointAddress(uint8_t address)
{
	watchpointsIterator = watchpoints.begin();
	while(watchpointsIterator != watchpoints.end())
	{
		if((*watchpointsIterator).second.first < address && (*watchpointsIterator).second.second > address)
			return true;
		std::advance(watchpointsIterator, 1);
	}
	return false;
}