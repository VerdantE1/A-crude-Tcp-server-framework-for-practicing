#include "TcpServer.h"
#include "Logger.h"
#include <cstring>
#include <string>
#include <functional>

/*********************************************************************
 WriteCompleteCallback = function<void(const TcpConnectionPtr&)>;
 MessageCallback = function<void(const TcpConnectionPtr&,
 ConnectionCallback = function<void(const TcpConnectionPtr&)>;

 ********************************************************************/
using namespace std;
using namespace placeholders;

class DayTimeServer
{
public:
    DayTimeServer(Control* reactor,
        const InetAddress& addr,
        const std::string& name)
        : server_(reactor, addr, name)
        , reactor_(reactor)
    {
        
        server_.setConnectionCallback(
            bind(&DayTimeServer::onConnection, this, std::placeholders::_1)
        );

        server_.setMessageCallback(
            bind(&DayTimeServer::onMessage, this,_1, _2, _3)
        );

        server_.setThreadNum(1);
    }
    void start()
    {
        server_.start();
    }
private:
    
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            //Coming doing
            LOG_INFO("Connection Coming : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            //Close doing
            LOG_INFO("Connection Closed: %s", conn->peerAddress().toIpPort().c_str());
        }
    }
    
    void onMessage(const TcpConnectionPtr& conn,
        Buffer* buf,
        Timestamp time)
    {
        //string msg = buf->retrieveAllAsString();  //Get the info from Buffer to msg
        string msg = Timestamp::now().toString();
        char buf1[50]; 
        memset(buf1, '\0', sizeof buf);
        strcat(buf1, msg.c_str());
        strcat(buf1, "\n");
        
        conn->send(buf1);  //send msg to peer sockfd
        conn->shutdown(); //shutdown write.this is half close. 
                          //If we active close the connection ,then we just shutdown on write (If the outBuffer still have datas,we wait the datas send done then shutdownwrite)
                          // , wait the peer's info coming and receive.
                          //And if we read 0 then we will call handleClose() ,and close the connection. 
    }

    Control* reactor_;
    TcpServer server_;
};

int main()
{

    //cout << Timestamp::now().toFormattedString() << endl;
    Control reactor;
    InetAddress addr(12346,"192.168.154.128");
    DayTimeServer server(&reactor, addr, "DayTimeServer-01"); 
    server.start(); 
    reactor.startloop(); 
    
    return 0;
}