#include <stdio.h>
#include "WatchpointException.h"

using namespace axe;

WatchpointException::WatchpointException(Type t, uint32_t addr) :
	address(addr),
	type(t)
{
	
}
