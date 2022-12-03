#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Handler.h"
#include "Control.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string>
#include <cassert>

static Control* CheckNotNull(Control *reactor)
{
    if (reactor == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection's Reactor is Null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return reactor;
}

TcpConnection::TcpConnection(Control *reactor, 
                const std::string &nameArg, 
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr)
    : reactor_(CheckNotNull(reactor))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , handler_(new Handler(reactor, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64*1024*1024) 
    ,input_m_read_idx_(inputBuffer_.Getreadidx())
    ,input_m_write_idx_(inputBuffer_.Getwriteidx())
    ,output_m_read_idx_(outputBuffer_.Getreadidx())
    ,output_m_write_idx_(outputBuffer_.Getwriteidx())
{
    
    handler_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );
    handler_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );
    handler_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this)
    );
    handler_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this)
    );

    LOG_DEBUG("TcpConnection[%s] which symbol on fd=%d", name_.c_str(), sockfd);

    socket_->setKeepAlive(true);
}


TcpConnection::~TcpConnection()
{
    assert(state_ == kDisconnected);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (reactor_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            reactor_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                buf.c_str(),
                buf.size()
            ));
        }
    }
}


void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    
    if (state_ == kDisconnected)
    {
        LOG_ERROR("Disconnected, give up writing!");
        return;
    }

    
    //first write:Directly Write to sockfd ,if reaming then send reaming data to outputBuffer
    if (!handler_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(handler_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                
                reactor_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
        }
        else 
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop have an Error");
                if (errno == EPIPE || errno == ECONNRESET) 
                {
                    faultError = true;
                }
            }
        }
    }
    
    if (!faultError && remaining > 0) 
    {
        
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            reactor_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen+remaining)
            );
        }

        //send reaming to output buffer
        outputBuffer_.append((char*)data + nwrote, remaining);
        if (!handler_->isWriting())
        {
            //Register the writing event,wait to next loop coming,then send outputbuffer's data to sockfd
            handler_->enableWriting(); 
        }
    }
}


void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        reactor_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this)
        );
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!handler_->isWriting()) 
    {
        socket_->shutdownWrite(); 
    }
}


void TcpConnection::connectEstablished()
{
    setState(kConnected);
    handler_->tie(shared_from_this());
    handler_->enableReading(); 

    
    connectionCallback_(shared_from_this());
}


void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        handler_->disableAll(); 
        connectionCallback_(shared_from_this());
    }
    handler_->remove(); 
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(handler_->fd(), &savedErrno);
    if (n > 0)
    {
        messageCallback_ (shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (handler_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(handler_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                handler_->disableWriting();
                if (writeCompleteCallback_)
                {
                    
                    reactor_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else         //n==0 &&  handler_->== true     
        {
            assert(outputBuffer_.readableBytes() == 0);
            handler_->disableWriting();
            if (writeCompleteCallback_)
            {
                reactor_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
            
        }
    }
    else
    {
        LOG_DEBUG("TcpConnection fd=%d is down, no more writing \n", handler_->fd());
    }
}


void TcpConnection::handleClose()
{
    LOG_DEBUG("TcpConnection::handleClose fd=%d state=%d \n", handler_->fd(), (int)state_);
    setState(kDisconnected);
    handler_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); 
    closeCallback_(connPtr); 
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(handler_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}