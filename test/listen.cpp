#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define IP "192.168.154.128"
#define PORT "12345"
#define BACK_LOGLENGTH 3

static bool stop = false;

static void handle_term(int sig)
{
    printf("将主循环退出 \n");
    stop = true;
}


int main(int argc, char* argv[])
{
    signal(SIGTERM, handle_term);

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

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, BACK_LOGLENGTH);
    assert(ret != -1);

    //循环等待监听队列，直到有一个连接
    while (!stop)
    {
        sleep(1);
    }
    

    //printf("监听完毕，准备关闭监听套接字，结束tcp服务器程序\n");
    close(sock);
    return 0;
}