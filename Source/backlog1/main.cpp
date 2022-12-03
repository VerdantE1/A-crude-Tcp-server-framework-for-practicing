#include "HTTPServer.h"
#include <cassert>
/*********************************************************************
 WriteCompleteCallback = function<void(const TcpConnectionPtr&)>;
 MessageCallback = function<void(const TcpConnectionPtr&,
 ConnectionCallback = function<void(const TcpConnectionPtr&)>;

 ********************************************************************/

int main()
{
    Control Main_Reactor;
    InetAddress addr(12346,"192.168.154.128");
    HttpServer Server(&Main_Reactor, addr, "Frank's HttpServer"); 



    Server.start();  //Start the subThreads and subReactor;
    Main_Reactor.startloop();  //IO thread start Loop;


   

    return 0;
}

