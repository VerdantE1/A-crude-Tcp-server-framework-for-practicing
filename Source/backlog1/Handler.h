#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class Control;

class Handler : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Handler(Control *reactor, int fd);
    ~Handler();

    
    void handleEvent(Timestamp receiveTime);  
  

    /***************Setting************************/
    void tie(const std::shared_ptr<void>&);         //Tie the TcpConnecion
    int set_revents(int revt) { revents_ = revt; }
    void set_index(int idx) { index_ = idx; }
    void enableReading() { events_ |= kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    //Setting cb ,TcpConnection And ListenExcutor do this
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }
    


    /******************State*************************/
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    int index() { return index_; }
    int fd() const { return fd_; }
    int events() const { return events_; }

    
    Control* ownerReactor() { return reactor_; }
    void remove();
private:

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    Control *reactor_; 
    const int fd_;    
    int events_; 
    int revents_; 
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

