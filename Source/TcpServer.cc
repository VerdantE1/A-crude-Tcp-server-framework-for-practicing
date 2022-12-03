#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>
#include <functional>



static Control* CheckNotNull(Control *reactor)
{
    if (reactor == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return reactor;
}

TcpServer::TcpServer(Control *reactor,
                const InetAddress &listenAddr,
                const std::string &nameArg,
                Option option)
                : Mainreactor_(CheckNotNull(reactor))
                , ipPort_(listenAddr.toIpPort())
                , name_(nameArg)
                , acceptor_(new ListenExcutor(reactor, listenAddr, option == kReusePort))
                , threadPool_(new WorkThreadPool(reactor, name_))
                , connectionCallback_()
                , messageCallback_()
                , nextConnId_(1)
                , started_(0)
{
    
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, 
        std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        
        TcpConnectionPtr conn(item.second); 
        item.second.reset();

        
        conn->GetReactor()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}


void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}


void TcpServer::start()
{
    if (started_++ == 0) 
    {
        threadPool_->start(threadInitCallback_); 
        Mainreactor_->runInLoop(std::bind(&ListenExcutor::listen, acceptor_.get()));
    }
}


void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    
    //1.Select Reactor
    Control *selectedreactor = threadPool_->getNextLoop(); 
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    LOG_DEBUG("TcpServer::newConnection [%s] - new connection [%s] from %s",  name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    
    //2.Get the sockfd's InetAddress
    sockaddr_in local;
    bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    

    //3.Creat a TcpConnection for cfd
    TcpConnectionPtr conn(new TcpConnection(selectedreactor, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    
    conn->setConnectionCallback(connectionCallback_);     //For cfd setting Eventhandle cb
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    
    conn->setCloseCallback( std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)); //Close Event is ready,not user setting
    selectedreactor->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));  //Give the functor to subreactor,and wake up subreactor's ownerthread
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    Mainreactor_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_DEBUG("TcpServer::removeConnectionInLoop [%s] - connection %s",   name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    Control * selectedreactor = conn->GetReactor();
    selectedreactor->queueInLoop( std::bind(&TcpConnection::connectDestroyed, conn) );
}