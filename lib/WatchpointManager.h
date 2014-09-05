#ifndef _WatchpointManager_h_
#define _WatchpointManager_h_

#include <cassert>
#include <set>

using namespace std;

typedef pair<unsigned int, unsigned int> Watchpoint;

class WatchpointManager {
private:
	set<Watchpoint> watchpoints;
	set<Watchpoint>::iterator watchpointsIterator;
	bool contains(Watchpoint w);
public:
	void SetWatchpoint(unsigned int lowAddr, unsigned int highAddr);
	void UnsetWatchpoint(unsigned int lowAddr, unsigned int highAddr);
	void ClearWatchpoints();

	bool checkWatchpointAt(unsigned int address);
};

#endif //_WatchpointManager_h_