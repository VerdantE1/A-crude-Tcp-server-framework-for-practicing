#pragma once
#include "TcpServer.h"
#include "Logger.h"
#include "TextProcess.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <string>
#include <functional>
#include <sys/uio.h>
#define DEBUG

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
        , INTERNAL_ERROR, CLOSED_CONNECTION,DIR_REQUEST
    };                                                 
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN }; //   Readline State
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };  //   HTTPRequest Method State






    HttpServer(Control* reactor, const InetAddress& addr, const std::string& name)
        : Server_(reactor, addr, name)
        , Reactor_(reactor)
    {
        Server_.setConnectionCallback(bind(&HttpServer::onConnection, this, _1));
        Server_.setMessageCallback(bind(&HttpServer::onMessage, this, _1, _2, _3));
        Server_.setWriteCompleteCallback(bind(&HttpServer::HttpVersion_WriteFd, this,_1));
        Server_.setThreadNum(4); //Set threads nums


    }
    ~HttpServer() {}    //stack variable,do not malloc it


    

    void start()
    {
        Server_.start(); //start subreactor and update lfdhandler
    }



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

    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time)
    {
        char* str = buf->peek();
   
        
        init();
        HTTP_CODE ret = Process_Read(conn);                     //Parse
        if (ret == NO_REQUEST) return;                          //NextLoop read
        assert(ret != NO_REQUEST);
        memset(buf->peek(), 0, buf->readableBytes());           //clear inputbuf
        buf->retrieveAll();
        check_state_ = CHECK_STATE_REQUESTLINE;                 //Must be Init State
        


        if (!Process_Write(conn,ret))
        {
            LOG_ERROR("Process_Write");
            return;
        }
        int ErrorSave;
        Buffer* outbuf = conn->GetoutputBuffer();
        outbuf->writeFd(conn->GetHandler()->fd(),&ErrorSave);   //send to sockfd
        outbuf->retrieveAll();
        if (!HttpVersion_WriteFd(conn))
        {
            close(conn->GetHandler()->fd());

        }


    }

    void onWriteComplete(const TcpConnectionPtr& conn)
    {
        
        if (!HttpVersion_WriteFd(conn))
        {
            close(conn->GetHandler()->fd());
        }
    }
    

    bool HttpVersion_WriteFd(const TcpConnectionPtr& conn);//HttpVersion Sendfile



    /***********************************Parse Request*******************************/
    HTTP_CODE Process_Read(const TcpConnectionPtr& conn);/* MainStateMachine */
    LINE_STATUS Parse_Line(const TcpConnectionPtr& conn);
    HTTP_CODE Parse_Content(char* text, const TcpConnectionPtr& conn);
    HTTP_CODE Parse_Request_Line(char* text, const TcpConnectionPtr& conn);
    HTTP_CODE Parse_Headers(char* text, const TcpConnectionPtr& conn);
    HTTP_CODE Do_Request(const TcpConnectionPtr& conn);
    char* get_line(const TcpConnectionPtr& conn);
    void unmap();
    void init();


    /**********************************Response***************************************/
    bool Process_Write(const TcpConnectionPtr& conn, HTTP_CODE ret);   //Write response to app buffer and setting iovec          
    bool Add_Status_Line(const TcpConnectionPtr& conn,int status,const char* title);
    bool Add_Headers(const TcpConnectionPtr& conn, int ContentLen);
    bool Add_Length(const TcpConnectionPtr& conn, int ContentLen);
    bool Add_Linger(const TcpConnectionPtr& conn );
    bool Add_Blank_Line(const TcpConnectionPtr& conn );
    bool Add_Content(const TcpConnectionPtr& conn, const char* Content);
    




    int Connectionscnt() { return Server_.Connectionscnts(); }

private:
    Control* Reactor_;              /* Main_Reactor */
    TcpServer Server_;              /* TcpServer */
    int start_line_;                /* Flag each line's start postion */

    CHECK_STATE check_state_;       /* Main_State Machine's state */
    METHOD method_;
    char real_file_[FILENAME_LEN];  /* Path of real file */
    char* url_;
    char* version_;                 /* For example: HTTP / 1.1 */
    char* host_;
    int content_length_;            /* Content Length */
    bool linger_;                   /* Long conncection */
    char* m_file_address_;          /* mmap:File mmap to a memory address */
    struct stat file_stat_;         /* File info */
    
    struct iovec m_iv[2];           /* Prevent complete datas  */
    int m_iv_count;
    
    int bytes_have_send=0;
    int cnt = 0;
};
