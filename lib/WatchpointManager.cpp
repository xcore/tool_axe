#include "WatchpointManager.h"
#include "Tracer.h"
#include <set>

using namespace axe;

void WatchpointManager::setWatchpoint(WatchpointType type, uint32_t lowAddr, uint32_t highAddr)
{
	Watchpoint w = Watchpoint(type, std::pair<uint32_t, uint32_t>(lowAddr, highAddr));
	watchpoints.insert(w);
}

void WatchpointManager::unsetWatchpoint(WatchpointType type, uint32_t lowAddr, uint32_t highAddr)
{
	Watchpoint w = Watchpoint(type, std::pair<uint32_t, uint32_t>(lowAddr, highAddr));
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
		if((*watchpointsIterator).first == t &&					// Check same type
		 	(*watchpointsIterator).second.first < address &&	// Check addr > lower bound
		  	(*watchpointsIterator).second.second > address)		// Check addr < upper bound
			return true;
		std::advance(watchpointsIterator, 1);
	}
	return false;
}