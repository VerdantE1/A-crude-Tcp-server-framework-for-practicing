#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class Control;
class WorkThread;

class WorkThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(Control*)>; 

    WorkThreadPool(Control *baseLoop, const std::string &nameArg);
    ~WorkThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    
    Control* getNextLoop();

    std::vector<Control*> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }
private:

    Control *baseLoop_; 
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<WorkThread>> threads_;
    std::vector<Control*> reactors_;
};