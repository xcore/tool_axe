#include "WatchpointManager.h"
#include "Tracer.h"
#include <set>

using namespace axe;

void WatchpointManager::setWatchpoint(uint8_t lowAddr, uint8_t highAddr)
{
	Watchpoint w = Watchpoint(lowAddr, highAddr);
	watchpoints.insert(w);
}

void WatchpointManager::unsetWatchpoint(uint8_t lowAddr, uint8_t highAddr)
{
	Watchpoint w = Watchpoint(lowAddr, highAddr);
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
		if((*watchpointsIterator).first < address && (*watchpointsIterator).second > address)
			return true;
		std::advance(watchpointsIterator, 1);
	}
	return false;
}