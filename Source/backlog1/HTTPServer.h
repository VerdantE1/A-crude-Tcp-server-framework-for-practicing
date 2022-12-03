#pragma once
#include "TcpServer.h"
#include "Logger.h"
#include "TextProcess.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <string>
#include <functional>



using namespace std;
using namespace placeholders;

class HttpServer
{
public:
    static const int FILENAME_LEN = 200; //   FileNamelength

    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };  //   Main_State Machine states
    enum HTTP_CODE {                                                   //  HTTPRequest results 
        NO_REQUEST, GET_REQUEST, BAD_REQUEST,
        NO_RESOURCE, FORBIDDEN_REQUET, FILE_REQUEST
        , INTERNAL_ERROR, CLOSED_CONNECTION
    };                                                 
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN }; //   Readline State
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };  //   HTTPRequest Method State






    HttpServer(Control* reactor, const InetAddress& addr, const std::string& name)
        : Server_(reactor, addr, name)
        , Reactor_(reactor)
    {
        Server_.setConnectionCallback(bind(&HttpServer::onConnection, this, _1));
        Server_.setMessageCallback(bind(&HttpServer::onMessage, this, _1, _2, _3));
        Server_.setThreadNum(0); //Set threads nums


    }
    ~HttpServer() {}    //stack variable,do not malloc it


    

    void start()
    {
        Server_.start(); //start subreactor and update lfdhandler
    }

    /*bool HTTP_Write();
    bool HTTP_Read();
    void HTTP_Process();*/


private:

    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection Coming: %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection Closed: %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn,
        Buffer* buf,
        Timestamp time)
    {
        auto p = buf->peek();
        printf("%s", p);
        init();
        Process_Read(conn);

        printf("1\n");
        conn->shutdown();
    }

   



    /*****************Called in onMessage() ¡ª¡ª Parse Request  ************************/
    void init();

    HTTP_CODE Process_Read(const TcpConnectionPtr& conn);                               /* MainStateMachine */
    LINE_STATUS Parse_Line(const TcpConnectionPtr& conn);
    HTTP_CODE Parse_Content(char* text, const TcpConnectionPtr& conn);
    HTTP_CODE Parse_Request_Line(char* text, const TcpConnectionPtr& conn);
    HTTP_CODE Parse_Headers(char* text, const TcpConnectionPtr& conn);
    HTTP_CODE Do_Request(const TcpConnectionPtr& conn);
    char* get_line(const TcpConnectionPtr& conn);



    //bool process_write();               //Fill HTTP_Response
    

    int Connectionscnt() { return Server_.Connectionscnts(); }

private:
    Control* Reactor_;              /* Main_Reactor */
    TcpServer Server_;              /* TcpServer */
    int start_line_;              /* Flag each line's start postion */

    CHECK_STATE check_state_;     /* Main_State Machine's state */
    METHOD method_;
    char real_file_[FILENAME_LEN];  /* Path of real file */
    char* url_;
    char* version_;                 /* For example: HTTP / 1.1 */
    char* host_;
    int content_length_;            /* Content Length */
    bool linger_;                   /* Long conncection */
    char* m_file_address_;          /* mmap:File mmap to a memory address */
    struct stat file_stat_;        /* File info */
    //static int m_user_count_;       /* Usercounts */
    
                     
};
