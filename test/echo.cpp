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

        int pipefd[2];
        assert(ret != -1);
        ret = pipe(pipefd);

        ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);

        assert(ret != -1);

        ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);

        close(connfd);

    }


    close(sock);
    return 0;
}