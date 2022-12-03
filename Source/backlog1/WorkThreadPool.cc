#include "WorkThreadPool.h"
#include "WorkThread.h"

#include <memory>

WorkThreadPool::WorkThreadPool(Control *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{}

WorkThreadPool::~WorkThreadPool()
{}

void WorkThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        WorkThread *t = new WorkThread(cb, buf);
        threads_.push_back(std::unique_ptr<WorkThread>(t));
        reactors_.push_back(t->startReactor()); 
    }

    
    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}


Control* WorkThreadPool::getNextLoop()
{
    Control *reactor = baseLoop_;

    if (!reactors_.empty()) 
    {
        reactor = reactors_[next_];
        ++next_;
        if (next_ >= reactors_.size())
        {
            next_ = 0;
        }
    }

    return reactor;
}

std::vector<Control*> WorkThreadPool::getAllLoops()
{
    if (reactors_.empty())
    {
        return std::vector<Control*>(1, baseLoop_);
    }
    else
    {
        reactors_;
    }
}