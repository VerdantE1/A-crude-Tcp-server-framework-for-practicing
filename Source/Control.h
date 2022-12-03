#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Handler;
class Poller;


class Control : noncopyable
{
public:
    using Functor = std::function<void()>;

    Control();
    ~Control();

    /********************The Core function******************************/
    void startloop();        //Run the Reactor
    void quit();        //Quit the Reactor
    void updateHandler(Handler* handler);            //Update the EventHandler
    void removeHandler(Handler* handler);            //Remove the EventHandler
    


    /*************Thread Sync mechanism************************/
    void runInLoop(Functor cb);     
    void queueInLoop(Functor cb);
    void wakeup();
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }


  
    Timestamp pollReturnTime() const { return pollReturnTime_; }
    bool HasHandler(Handler *handler);

    
private:
    void handleRead(); 
    void doPendingFunctors(); 

    using HandlerList = std::vector<Handler*>;

    std::atomic_bool looping_;  
    std::atomic_bool quit_; 
    
    const pid_t threadId_;      //The Onwer Thread's PID

    Timestamp pollReturnTime_; 
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; 
    std::unique_ptr<Handler> WakeHandler_;

    HandlerList ActiveHandlers_;

    std::atomic_bool callingPendingFunctors_; 
    std::vector<Functor> pendingFunctors_;           //After the Handler's things done,then do this functors.
    std::mutex mutex_; 
};