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
#define BUFFER_SIZE 1024
const char* file_name = nullptr;

//状态行
static const char* status_line[2] = { "200 OK","500 Internal Server Error" };

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


    sleep(2);

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



        //存储头文件
        char header_buf[BUFFER_SIZE];
        memset(header_buf, 0, sizeof header_buf);


        /*静态开辟*/
        /*发送HTTP响应头 和 资源 */

        //用于存放应用层缓冲区
        char* file_buf;
        //用于获取目标文件的属性，比如是否为目录，文件大小等
        struct stat file_stat;

        //记录目标文件是否是有效文件
        bool valid = false;

        //缓存区header_buf目前已经使用了多少字节的空间
        int len = 0;




        /* 逻辑1：判断目标文件是否有效。 先判断是否存在若存在，判断是否是目录 或 普通文件 */
        if (stat(file_name, &file_stat) < 0)  /* 目标文件不存在 */
        {
            valid = false;
        }

        else
        {
            if (S_ISDIR(file_stat.st_mode))             //若是目录则表示无效
            {
                valid = false;
            }
            else if (file_stat.st_mode & S_IROTH)         //若是普通文件，则先判读是否有读取目标文件的权限，过来申请资源的肯定是其他组的
            {
                int fd = open(file_name, O_RDONLY);       //只读打开文件
                file_buf = new char[file_stat.st_size + 1];           //静态开辟文件资源空间;
                memset(file_buf, 0, sizeof file_buf);
                if (read(fd, file_buf, file_stat.st_size) < 0)         //阻塞式读取
                {
                    valid = false;
                }
            }
            else                                              //其他情况表示不可访问
            {
                valid = false;
            }
        }

        /* 逻辑2：如果目标文件有效，则发送正常的HTTP应答 */
        if (valid)
        {
            //首先发送正常的HTTP应答。状态行，"Content-Lenght"头部字段，空字段
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[0]); len += ret;

            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "Content-Length: %d\r\n", file_stat.st_size); len += ret;

            ret = snprintf(header_buf + len, BUFFER_SIZE - len - 1, "\r\n"); len += ret;

            //其次再利用writev将header_buf和file_buf的内容一并写出
            struct iovec iv[2];
            iv[0].iov_base = header_buf;
            iv[0].iov_len = strlen(header_buf);

            iv[1].iov_base = file_buf;
            iv[1].iov_len = file_stat.st_size;

            ret = writev(connfd, iv, 2);

        }
        else   //如果文件无效，则通知对端发生了内部错误
        {
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[1]);  len += ret;

            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "%s", "\r\n");

            send(connfd, header_buf, strlen(header_buf), 0);

        }
        close(connfd);
        if (valid == true) delete[] file_buf;
    }


    close(sock);
    return 0;
}