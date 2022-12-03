#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 4096
class tw_timer;

class client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    tw_timer* timer;
};

/*  定时器类   */
class tw_timer
{
public:
    tw_timer(int rot, int ts) :rotation(rot), time_slot(ts),next(nullptr),prev(nullptr) {}
    ~tw_timer() {}
    

public:
    int rotation;               //记录定时器第几轮
    int time_slot;               //记录定时器属于时间轮上哪个槽
    void (*cb_func)(client_data*); //定时器回调函数
    client_data* user_data;        //客户数据
    tw_timer* next ;                //索引结构
    tw_timer* prev;
};


class time_wheel
{
public:
    time_wheel() :cur_slot(0)
    {
        for (int i = 0; i < N; i++)
        {
            slots[i] = nullptr;
        }
    }

    ~time_wheel()
    {
        for (int i = 0; i < N; i++)
        {
            tw_timer* tmp = slots[i];
            while (tmp)
            {
                slots[i] = tmp->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }

    tw_timer* add_timer(int timeout)
    {
        if (timeout < 0)
        {
            return nullptr;
        }

        /* 计算滴答数 */
        int ticks = 0;
        if (timeout < SI) {
            ticks = 1;
        }
        else
        {
            ticks = (timeout / SI);
        }

        /* 计算第几个槽，第几轮 */
        int rotation = ticks / N;
        int ts = (cur_slot + (ticks % N)) % N;
        

        /* 创建新的定时器并插入 */
        tw_timer* timer = new tw_timer(rotation, ts);
        if (!slots[ts])
        {
            printf("add timer,rotation is %d,ts is %d ,cur_slot is %d\n", rotation, ts, cur_slot);
            slots[ts] = timer;
        }
        else
        {
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;

    }

    /* 删除目标定时器timer */
    void del_timer(tw_timer* timer)
    {
        if (!timer)
        {
            return;
        }
        int ts = timer->time_slot;

        if (timer == slots[ts])
        {
            slots[ts] = slots[ts]->next;
            if (slots[ts])
            {
                slots[ts]->prev = nullptr;
            }
            delete timer;
        }
        else
        {
            timer->prev->next = timer->next;
            if(timer ->next ) timer->next->prev = timer->prev;
            delete timer;
        }
    }

    /* SI时间到后，发出滴答，时间轮动一个槽，并执行回调 */
    void tick()
    {
        tw_timer* tmp = slots[cur_slot];
        printf("current slot is %d\n", cur_slot);
        while (tmp)
        {
            printf("tick the timer once\n");
            if (tmp->rotation > 0)
            {
                tmp->rotation--;
                tmp = tmp->next;
            }
            else
            {
                tmp->cb_func(tmp->user_data);
                if (tmp == slots[cur_slot])
                {
                    printf("delete header in cur_slot\n");
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if (slots[cur_slot])
                    {
                        slots[cur_slot]->prev = NULL;
                    }
                    tmp = slots[cur_slot];
                }
                else
                {
                    tmp->prev->next = tmp->next;
                    if (tmp->next)
                    {
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer* tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        cur_slot = ++cur_slot % N;
    }



public:
    static const int N = 60; //时间轮上的槽数
    static const int SI = 1; //每1s时间轮转动一次，即槽间隔为1s
    tw_timer* slots[N];   //时间轮的槽，其中每个元素指向一个定时器链表，链表无序
    int cur_slot;       //时间轮的当前槽
};
