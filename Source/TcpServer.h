#pragma once

#include "Control.h"
#include "ListenExcutor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "WorkThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>




//using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
//using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
//using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
//using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
//using MessageCallback = std::function<void(const TcpConnectionPtr&,
//    Buffer*,
//    Timestamp)>;
//using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;





class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(Control*)>;
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;


    enum Option
    {
        kNoReusePort,
        kReusePort,
    };


    TcpServer(Control * Mainreactor_,const InetAddress &listenAddr, const std::string &nameArg, Option option = kNoReusePort);
    ~TcpServer();



    /********************Setting Events cb***********************************/
    void setThreadInitcallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }       
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    /********Set Threads Number*************************/
    void setThreadNum(int numThreads);

    /***********Run*********************/
    void start();    //Run the subReactor but not run the MainReactor And update the LfdHandler in Main Reactor

    int Connectionscnts() { return connections_.size(); } 
private:
    void newConnection(int sockfd, const InetAddress &peerAddr);  //For ListenExcuter
    void removeConnection(const TcpConnectionPtr &conn);          //For any Handler's close event
    void removeConnectionInLoop(const TcpConnectionPtr &conn);


    Control *Mainreactor_; 

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<ListenExcutor> acceptor_;
    std::shared_ptr<WorkThreadPool> threadPool_; 

    ConnectionCallback connectionCallback_; 
    MessageCallback messageCallback_; 
    WriteCompleteCallback writeCompleteCallback_; 
    ThreadInitCallback threadInitCallback_; 

    std::atomic_int started_;

    int nextConnId_; //Connection nums
    ConnectionMap connections_;  //Manage the Connections
};