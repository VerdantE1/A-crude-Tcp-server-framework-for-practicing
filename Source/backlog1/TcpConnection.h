#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Handler;
class Control;
class Socket;


class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(Control *reactor, 
                const std::string &name, 
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    Control* GetReactor() const { return reactor_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }





    /**********Send info to Application's Buffer ******************/
    void send(const std::string &buf);
    

    /*************Setting Handler's Callback****************************/
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

    

    void shutdown();
    void connectEstablished();
    void connectDestroyed();

    /***************Get Buffer*********************/
    Buffer* GetoutputBuffer() { return &outputBuffer_; }
    Buffer* GetinputBuffer() { return &inputBuffer_; }

    size_t& Getoutput_readidx() { return output_m_read_idx_; }
    size_t& Getoutput_writeidx() { return output_m_write_idx_; }
    size_t& Getinput_readidx() { return input_m_read_idx_; }
    size_t& Getinput_writeidx() { return input_m_write_idx_; }


private:
    enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    Control *reactor_; 
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Handler> handler_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; 
    MessageCallback messageCallback_; 
    WriteCompleteCallback writeCompleteCallback_; 
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;  
    Buffer outputBuffer_; 

    size_t& input_m_read_idx_;
    size_t& input_m_write_idx_;
    size_t& output_m_read_idx_;
    size_t& output_m_write_idx_;
};