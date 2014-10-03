#ifndef __WatchpointException_
#define __WatchpointException_

#include <stdint.h>

namespace axe {
	class WatchpointException {
	private:
		uint32_t address;
	public:
		enum Type {
			READ,
			WRITE
		};
		Type type;
		WatchpointException(Type t, uint32_t addr);
		uint32_t getAddr() { return address; }
	};
}

#endif // __WatchpointException_