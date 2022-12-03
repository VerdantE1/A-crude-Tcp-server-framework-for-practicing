#include "include/Com_Net.h"
#include "include/Com_Base.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>



#define IP "192.168.154.128"
#define PORT "12346"
#define BACK_LOGLENGTH 3
#define BUFFER_SIZE 4096
const char* file_name = nullptr;
/* 主状态机的两种可能状态表示，分别表示：当前正在分析请求行,当前正在分析头部字段 */
enum CHECK_STATUS { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER };

/* 从状态机有三种可能状态表示,即行的读取状态，分别表示：读取到一个完整的行，行出错 以及 行数据尚且不完整*/
enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

/* 服务器处理HTTP请求的结果：NO_REQUEST表示请求不完整，需要继续读取客户数据;GET_REQUEST表示获得了一个完整的客户请求;BAD_REQUEST表示客户端请求有语法错误;
   FORBIDDEN_REQUEST表示客户对资源没有足够的权限访问;INTERNAL_ERROR表示服务器的内部错误;CLOSED_CONNECTION表示客户端已经关闭连接了                   */

enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, FORBIDDEN_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };


//DEBUG暂时测试
const char* RETURN[] = { "发送结果\n","获取状态错误\n" };


/*从状态机,解析行，返回从状态机读取行的状态 */
LINE_STATUS parse_line(char* buffer, int& checked_index, int& read_index)
{
    char temp; //读取的每一个字符

    //一直往下读直到遇到\r 或 \n
    for (; checked_index < read_index; checked_index++)
    {
        temp = buffer[checked_index];

        if (temp == '\r')
        {
            //只有一个\r是不够的，必须以\r\n结尾，此时正好用户的数据没发过来
            if (checked_index + 1 == read_index)
            {
                return LINE_OPEN;
            }

            else if (buffer[checked_index + 1] = '\n')
            {
                buffer[checked_index] = '\0';
                buffer[++checked_index] = '\0';
                return LINE_OK;
            }
            else
                return LINE_BAD;

        }
        else if (temp == '\n')
        {
            if (buffer[checked_index - 1] == '\r')
            {
                buffer[checked_index - 1] = '\0';
                buffer[checked_index] = '\0';
                return LINE_OK;
            }
            else
                return LINE_BAD;
        }

    }
    //所有字符都读完了但是仍没遇到\r或\n字符，返回LINE_OPEN，目前的数据是不够分析的，需要继续读取数据分析
    return LINE_OPEN;


}

/* 行读取完后，就要开始分是请求行还是头部字段      */
//解析请求行，返回HTTP解析状态
HTTP_CODE parse_requestline(char* temp, CHECK_STATUS& checkstate)
{
    char* url = strpbrk(temp, " \t");
    if (url == nullptr)
    {
        return BAD_REQUEST;
    }
    *url++ = '\0';
    url += strspn(url, " \t");


    char* version = strpbrk(url, " \t");
    if (version == nullptr)
    {
        return BAD_REQUEST;
    }
    *version = '\0';



    if (strcasecmp(temp, "GET") != 0)
    {
        return BAD_REQUEST;
    }
    if (strcasecmp(url, "http://") == 0)
    {
        url += 7;
        url = strchr(url, '/');
    }
    if (strcasecmp(version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }

    if (!url || url[0] != '/')
    {
        return BAD_REQUEST;
    }

    printf("The request URL is : %s\n", url);

    //请求行处理完毕，驱动主状态机的状态
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;

}




//解析头部字段
HTTP_CODE parse_headers(char* temp)
{

    if (temp[0] == '\0') return GET_REQUEST;        //遇到空行,返回解析成功

    else if (strcasecmp(temp, "HOST:") == 0)
    {
        temp += 5;
        temp += strspn(temp, " \t");
        printf("the request host is : %s \n", temp);
    }
    else //其他头部字段都先不处理
    {
        printf("i cant handler it \n");
    }

    return NO_REQUEST;
}






/* 分析HTTP请求的入口函数 */
HTTP_CODE parse_content(char* buffer, int& checked_index, CHECK_STATUS& checkstate, int& read_index, int& start_line)
{
    LINE_STATUS linestatus = LINE_OK;
    HTTP_CODE retcode = NO_REQUEST;

    while ((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK)    //LINE_OK则进入
    {
        char* Linetemp = buffer + start_line;
        start_line = checked_index;

        //主状态机的驱动
        switch (checkstate)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            retcode = parse_requestline(Linetemp, checkstate);
            if (retcode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER:
        {
            retcode = parse_headers(Linetemp);
            if (retcode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }

        default:
        {
            return INTERNAL_ERROR;
        }
        }

    }

    //若进入LINE_OPEN
    if (linestatus == LINE_OPEN)
    {
        return NO_REQUEST;
    }

    //LINE_BAD
    else
    {
        return BAD_REQUEST;
    }

}





int main(int argc, char* argv[])
{

    //创建一个IPV4 socket地址容器。     1.初始化 2.设置ipv4地址(注意转为网络端)
    const char* ip = IP;
    int port = atoi(PORT);
    struct sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);


    //建立一个socket用以监听。   1.创建一个socket 2.将socket绑定监听端口转化为listen socket 并赋于监听队列长度  3.监听，等待连接
    int sock = socket(PF_INET, SOCK_STREAM, 0); //ipv4,tcp,0(默认为0即可，前两个值确定了是什么协议)
    assert(sock >= 0);
    //设置端口复用
    SetReuse(sock);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, BACK_LOGLENGTH);
    assert(ret != -1);




    //用一个容器存储远端地址，由OS写故要指定长度。
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0)
    {
        printf("errno is :%d\n", errno);
    }
    else
    {
        char remote[INET_ADDRSTRLEN];
        printf("远端:IP:%s PORT:%d 已经连接\n", inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN), ntohs(client.sin_port));

        char buffer[BUFFER_SIZE];  //读缓冲区
        memset(buffer, 0, sizeof buffer);
        int ret = 0;
        int read_index = 0;    //已经读完了多少个字节
        int checked_index = 0; //已经分析完了多少个字节
        int start_line = 0;   //下一行的下标

        CHECK_STATUS checkstate = CHECK_STATE_REQUESTLINE;

        while (1)
        {
            ret = recv(connfd, buffer + read_index, BUFFER_SIZE - read_index, 0);
            if (ret == -1)
            {
                printf("读取失败\n");
                break;
            }
            else if (ret == 0)
            {
                //printf("对端关闭 \n");
                close(connfd);
                break;
            }
            read_index += ret;

            HTTP_CODE result = parse_content(buffer, checked_index, checkstate, read_index, start_line);

            if (result == NO_REQUEST)
            {
                continue;
            }
            else if (result == GET_REQUEST)
            {
                //业务逻辑
                send(connfd, RETURN[0], strlen(RETURN[0]), 0);
            }
            else
            {
                send(connfd, RETURN[1], strlen(RETURN[1]), 0);
            }



        }
        close(connfd);

    }


    close(sock);
    return 0;
}