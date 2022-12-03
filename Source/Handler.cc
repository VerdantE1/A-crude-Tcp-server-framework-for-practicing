#include "Handler.h"
#include "Control.h"
#include "Logger.h"
#include <sys/epoll.h>



const int Handler::kNoneEvent = 0;
const int Handler::kReadEvent = EPOLLIN | EPOLLPRI;
const int Handler::kWriteEvent = EPOLLOUT;


Handler::Handler(Control *reactor, int fd)
    : reactor_(reactor), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Handler::~Handler()
{
}


void Handler::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}


void Handler::update()
{
    
    reactor_->updateHandler(this);
}


void Handler::remove()
{
    reactor_->removeHandler(this);
}


void Handler::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}


void Handler::handleEventWithGuard(Timestamp receiveTime)
{
     LOG_DEBUG("Handler handleEvent revents:%d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}