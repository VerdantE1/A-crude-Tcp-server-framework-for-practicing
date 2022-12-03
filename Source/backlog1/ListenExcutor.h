#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Handler.h"
#include <functional>

class Control;
class InetAddress;

class ListenExcutor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    ListenExcutor(Control *reactor, const InetAddress &listenAddr, bool reuseport);
    ~ListenExcutor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) 
    {
        newConnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_; }
    void listen();
private:
    void handleRead();
    
    Control *reactor_; 
    Socket acceptSocket_;
    Handler acceptHandler_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};