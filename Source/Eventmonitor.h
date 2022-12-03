#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Handler;


class Eventmonitor : public Poller
{
public:
    Eventmonitor(Control *reactor);
    ~Eventmonitor() override;

    
    /*************************The Core function******************************************/
    Timestamp poll(int timeoutMs, HandlerList* ActiveHandlers) override;        //Run the Demultiplex And Return the ActiveHandlers to Reactor (Override the Poller)
    void updateHandler(Handler *handler) override;                              //Called by Reactor.Creat the Handler in this EventDemultplex,the fd will be register on epoll tree
    void removeHandler(Handler *handler) override;                              //Called by Reactor.Delete the Handler in this EventDemultplex

private:
    static const int kInitEventListSize = 16;

    
    void FillActiveHandlers(int numEvents, HandlerList *ActiveHandlers) const;
    
    void update(int operation, Handler *handler);

    using EventList = std::vector<epoll_event>;                                //The Ready Event returned by epoll are managed in this vector (ev.fd == fd .   ev.ptr == *handler) 

    int epollfd_;
    EventList events_;
};