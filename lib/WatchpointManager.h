#ifndef _WatchpointManager_h_
#define _WatchpointManager_h_

#include <cassert>
#include <set>
#include <stdint.h>

namespace axe {

enum class WatchpointType {
	READ,
	WRITE
};

typedef std::pair<WatchpointType, std::pair<uint32_t, uint32_t>> Watchpoint;

class WatchpointManager {
private:
	std::set<Watchpoint> watchpoints;
	std::set<Watchpoint>::iterator watchpointsIterator;
	bool contains(Watchpoint w);
public:
	void setWatchpoint(WatchpointType type, uint32_t lowAddr, uint32_t highAddr);
	void unsetWatchpoint(WatchpointType type, uint32_t lowAddr, uint32_t highAddr);
	void clearWatchpoints() { watchpoints.clear(); }
	int size() { return watchpoints.size(); }

	bool isWatchpointAddress(WatchpointType t, uint32_t address);
};

}; // End axe namespace

#endif //_WatchpointManager_h_
