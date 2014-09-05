#ifndef _WatchpointManager_h_
#define _WatchpointManager_h_

#include <cassert>
#include <set>
#include <stdint.h>

typedef std::pair<uint8_t, uint8_t> Watchpoint;

class WatchpointManager {
private:
	std::set<Watchpoint> watchpoints;
	std::set<Watchpoint>::iterator watchpointsIterator;
	bool contains(Watchpoint w);
public:
	void setWatchpoint(uint8_t lowAddr, uint8_t highAddr);
	void unsetWatchpoint(uint8_t lowAddr, uint8_t highAddr);
	void clearWatchpoints() { watchpoints.clear(); }

	bool isWatchpointAddress(uint8_t address);
};

#endif //_WatchpointManager_h_