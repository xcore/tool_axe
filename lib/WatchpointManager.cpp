#include "WatchpointManager.h"
#include "Tracer.h"
#include <set>

using namespace axe;

void WatchpointManager::SetWatchpoint(unsigned int lowAddr, unsigned int highAddr)
{
	Watchpoint w = Watchpoint(lowAddr, highAddr);
	watchpoints.insert(w);
}

void WatchpointManager::UnsetWatchpoint(unsigned int lowAddr, unsigned int highAddr)
{
	Watchpoint w = Watchpoint(lowAddr, highAddr);
}

bool WatchpointManager::contains(Watchpoint w)
{
	watchpointsIterator = watchpoints.find(w);
	return !(watchpointsIterator == watchpoints.end());
}