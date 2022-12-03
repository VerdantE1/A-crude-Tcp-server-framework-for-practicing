#include "Eventmonitor.h"
#include "Logger.h"
#include "Handler.h"
#include "CurrentThread.h"
#include <errno.h>
#include <unistd.h>
#include <strings.h>


const int kNew = -1;  
const int kAdded = 1;
const int kDeleted = 2;



Eventmonitor::Eventmonitor(Control *reactor)
    : Poller(reactor)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)  
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

Eventmonitor::~Eventmonitor() 
{
    ::close(epollfd_);
}

Timestamp Eventmonitor::poll(int timeoutMs, HandlerList *ActiveHandlers)
{
    
    LOG_DEBUG("Thread:%d 's Eventmonitor::Poll has started.Countsfd:%lu",CurrentThread::tid(), handlers_.size());

    int numEvents = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_DEBUG("%d Handlers are ready\n", numEvents);
        FillActiveHandlers(numEvents, ActiveHandlers);
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("Eventmonitor::Poll Timeout\n");
    }
    else
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("Eventmonitor::Poll Error!\n");
        }
    }
    return now;
}


void Eventmonitor::updateHandler(Handler *handler)
{
    //index == New  || index == Deleted -> EPOLLADD / index == Inserted ->EPOLLCTL
    const int index = handler->index();
    LOG_DEBUG("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, handler->fd(), handler->events(), index);

    //index == New || index == Deleted
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = handler->fd();
            handlers_[fd] = handler;
        }

        handler->set_index(kAdded);
        update(EPOLL_CTL_ADD, handler);
    }
    //index == Inserterd
    else  
    {
        int fd = handler->fd();
        if (handler->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, handler);
            handler->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, handler);
        }
    }
}


void Eventmonitor::removeHandler(Handler *handler) 
{
    int fd = handler->fd();
    handlers_.erase(fd);

    LOG_DEBUG("func=%s => fd=%d\n", __FUNCTION__, fd);
    
    int index = handler->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, handler);
    }
    handler->set_index(kNew);
}


void Eventmonitor::FillActiveHandlers(int numEvents, HandlerList *ActiveHandlers) const
{
    for (int i=0; i < numEvents; ++i)
    {
        Handler *handler = static_cast<Handler*>(events_[i].data.ptr);
        handler->set_revents(events_[i].events);
        ActiveHandlers->push_back(handler); 
    }
}


void Eventmonitor::update(int operation, Handler *handler)
{
    epoll_event event;
    bzero(&event, sizeof event);
    
    int fd = handler->fd();
    event.events = handler->events();
    event.data.fd = fd; 
    event.data.ptr = handler;
    

    if (epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("Epoll_ctl Del Error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("Epoll_ctl Add/Mod Error:%d\n", errno);
        }
    }
}