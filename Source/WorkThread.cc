#include "WorkThread.h"
#include "Control.h"


WorkThread::WorkThread(const ThreadInitCallback &cb, 
        const std::string &name)
        : reactor_(nullptr)
        , exiting_(false)
        , thread_(std::bind(&WorkThread::threadFunc, this), name)
        , mutex_()
        , cond_()
        , callback_(cb)
{

}

WorkThread::~WorkThread()
{
    exiting_ = true;
    if (reactor_ != nullptr)
    {
        reactor_->quit();
        thread_.join();
    }
}

Control* WorkThread::startReactor()
{
    thread_.start(); 

    Control *reactor = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while ( reactor_ == nullptr )
        {
            cond_.wait(lock);
        }
        reactor = reactor_; 
    }
    return reactor;
}


void WorkThread::threadFunc()
{
    Control reactor; 

    if (callback_)
    {
        callback_(&reactor);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        reactor_ = &reactor; //give the subreactor's pointer to manager
        cond_.notify_one(); //Tell the main thread
    }

    reactor.startloop(); 
    std::unique_lock<std::mutex> lock(mutex_);
    reactor_ = nullptr;
}