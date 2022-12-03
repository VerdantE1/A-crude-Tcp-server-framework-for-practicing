#include "Poller.h"
#include "Handler.h"

Poller::Poller(Control *reactor)
    : OwnerReactor_(reactor)
{
}

bool Poller::HasHandler(Handler *handler) const
{
    auto it = handlers_.find(handler->fd());
    return it != handlers_.end() && it->second == handler;
}