#ifndef __WatchpointException_
#define __WatchpointException_

#include <stdint.h>
#include "Resource.h"

namespace axe {
	class Thread;
	class WatchpointException {
	private:
		uint32_t address;
	public:
		enum Type {
			READ,
			WRITE
		};
		Type type;
		Thread &thread;
		ticks_t time;
		WatchpointException(Type t, uint32_t addr, Thread &thrd, ticks_t tm) :
			thread(thrd), address(addr), type(t), time(tm) {}
		uint32_t getAddr() { return address; }
		ticks_t getTime() const { return time; }
		Thread &getThread() const { return thread; } 
	};
}

#endif // __WatchpointException_