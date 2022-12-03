#include "Poller.h"
#include "Eventmonitor.h"
#include <stdlib.h>

Poller* Poller::newDefaultPoller(Control *reactor)
{
        return new Eventmonitor(reactor); 
}