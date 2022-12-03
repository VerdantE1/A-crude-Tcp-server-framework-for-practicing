#include "Control.h"
#include "Logger.h"
#include "Poller.h"
#include "Handler.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>


__thread Control *t_reactorInThisThread = nullptr;

#ifdef POLLTIMEOUTMS
const int kPollTimeMs = 100000;
#else
const int kPollTimeMs = -1;
#endif 





int createEventfd()                         //Threads communications fd
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}



Control::Control()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , WakeHandler_(new Handler(this, wakeupFd_))
{
    LOG_DEBUG("Control created %p in thread %d \n", this, threadId_);
    if (t_reactorInThisThread)
    {
        LOG_FATAL("Another Control %p exists in this thread %d \n", t_reactorInThisThread, threadId_);
    }
    else
    {
        t_reactorInThisThread = this;
    }

    
    WakeHandler_->setReadCallback(std::bind(&Control::handleRead, this));
    WakeHandler_->enableReading(); //wakeHandler register in Loop
}

Control::~Control()
{
    WakeHandler_->disableAll();
    WakeHandler_->remove();
    close(wakeupFd_);
    t_reactorInThisThread = nullptr;
}


void Control::startloop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("Control:%p ,Thread:%d Start reactoring", this,CurrentThread::tid());

    while(!quit_)
    {
        ActiveHandlers_.clear();
        
        pollReturnTime_ = poller_->poll(kPollTimeMs, &ActiveHandlers_);
        for (Handler *handler : ActiveHandlers_)
        {
            
            handler->handleEvent(pollReturnTime_);
        }
        

        doPendingFunctors();
    }

    LOG_INFO("Control %p Stop reactoring. \n", this);
    looping_ = false;
}



void Control::quit()
{
    quit_ = true;

    
    if (!isInLoopThread())  
    {
        wakeup();
    }
}


void Control::runInLoop(Functor cb)
{
    if (isInLoopThread()) 
    {
        cb();
    }
    else 
    {
        queueInLoop(cb);
    }
}

void Control::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    
    
    if (!isInLoopThread() || callingPendingFunctors_) 
    {
        wakeup(); 
    }
}

void Control::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("Control::handleRead() reads %lu bytes instead of 8", n);
  }
}


void Control::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("Control::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}


void Control::updateHandler(Handler *handler)
{
    poller_->updateHandler(handler);
}

void Control::removeHandler(Handler *handler)
{
    poller_->removeHandler(handler);
}

bool Control::HasHandler(Handler *handler)
{
    return poller_->HasHandler(handler);
}

void Control::doPendingFunctors() 
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); 
    }

    callingPendingFunctors_ = false;
}