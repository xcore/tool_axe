#ifndef _WatchpointManager_h_
#define _WatchpointManager_h_

#include <cassert>
#include <set>
#include <stdint.h>

enum WatchpointType {
	AXE_WATCH_TYPE_READ,
	AXE_WATCH_TYPE_WRITE,
};

typedef std::pair<WatchpointType, std::pair<uint8_t, uint8_t>> Watchpoint;

class WatchpointManager {
private:
	std::set<Watchpoint> watchpoints;
	std::set<Watchpoint>::iterator watchpointsIterator;
	bool contains(Watchpoint w);
public:
	void setWatchpoint(WatchpointType type, uint8_t lowAddr, uint8_t highAddr);
	void unsetWatchpoint(WatchpointType type, uint8_t lowAddr, uint8_t highAddr);
	void clearWatchpoints() { watchpoints.clear(); }
	int size() { return watchpoints.size(); }

	bool isWatchpointAddress(uint8_t address);
};

#endif //_WatchpointManager_h_