#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Handler;
class Control;


class Poller : noncopyable
{
public:
    using HandlerList = std::vector<Handler*>;

    Poller(Control *reactor);
    virtual ~Poller() = default;

    
    virtual Timestamp poll(int timeoutMs, HandlerList *ActiveHandlers) = 0;
    virtual void updateHandler(Handler *handler) = 0;
    virtual void removeHandler(Handler *handler) = 0;
    
    
    bool HasHandler(Handler *handler) const;

    
    static Poller* newDefaultPoller(Control *reactor);
protected:
    
    using HandlerMap = std::unordered_map<int, Handler*>;
    HandlerMap handlers_;
private:
    Control *OwnerReactor_; 
};