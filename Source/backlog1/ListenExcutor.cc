#include "ListenExcutor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>    
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>


static int createNonblocking()
{
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) 
    {
        LOG_FATAL("ListenExcutor::createNonblocking err:%d \n",  errno);
    }
}

ListenExcutor::ListenExcutor(Control *reactor, const InetAddress &listenAddr, bool reuseport)
    : reactor_(reactor)
    , acceptSocket_(createNonblocking()) 
    , acceptHandler_(reactor, acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); 
    acceptHandler_.setReadCallback(std::bind(&ListenExcutor::handleRead, this));

}

ListenExcutor::~ListenExcutor()
{
    acceptHandler_.disableAll();
    acceptHandler_.remove();
}

void ListenExcutor::listen()
{
    listenning_ = true;
    acceptSocket_.listen(); 
    acceptHandler_.enableReading(); 
}


void ListenExcutor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr); 
        }
        else
        {
            close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d Accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}