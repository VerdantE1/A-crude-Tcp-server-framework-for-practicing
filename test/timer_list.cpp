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


class client_data;

/* 定时器类 */
class Timer
{
public:
    using TimerHandler = void(*)(client_data*);
    Timer(TimerHandler cb) :prev(nullptr), next(nullptr), time_cb(cb) {};
    ~Timer() {};
   
public:
    Timer* prev; //索引结构
    Timer* next;
    TimerHandler time_cb = nullptr;  //回调func
    time_t expire; //超时时间
    client_data* user_data; 
};




/* 升序链表 */
class Sort_Timer_List
{
public:
    Sort_Timer_List() :head(nullptr), tail(nullptr) {};
    ~Sort_Timer_List()
    {
        Timer* tmp = head;
        while (tmp)
        {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    };




    /* 将目标定时器加入到升序链表中 */
    void Add_Timer(Timer* timer)
    {
        if (timer == nullptr)
        {
            return;
        }

        if (head == nullptr)  //该链表没有结点时
        {
            head = tail = timer;
            return;
        }

        if (timer->expire < head->expire)    //该链表有结点时，但是timer的时间最小
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }

        Add_Timer_After(timer, head);


    }


    /* 调整定时器：当某个定时器的任务改变了需要调整链表，只提供调整更大的时间接口   */
    void Addjust_Timer(Timer* timer)
    {
        if (timer == nullptr)
        {
            return;
        }
        Timer* tmp = timer->next;

        if (timer == tail || (head == tail && head == timer) || timer->expire < tmp->expire) return; //此时timer位于tail，或链表中只有一个结点 或 仍小于下一个结点的超时值 不用调整


        /*  此后的链表结点均至少大于等于2，timer不是尾结点, timer的超时值一定大于其后继结点    */

        //若timer是头结点
        if (head == timer)
        {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            Add_Timer_After(timer, head);

        }

        //若timer不是头节点
        else
        {
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            Add_Timer_After(timer, timer->next);

        }
    }


    /* 将目标定时器Timer从链表删除  */
    void Del_Timer(Timer* timer)
    {
        if (timer == nullptr)
        {
            return;
        }

        //一个结点的情况
        if ((timer == head) && (head == tail))
        {
            head = nullptr;
            tail = nullptr;
            delete timer;
            return;
        }


        //下面是两个以上结点的情况

        if (timer == head)
        {
            head = timer->next;
            head->prev = nullptr;
            delete timer;
            return;
        }

        if (timer == tail)
        {
            tail = timer->prev;
            tail->next = nullptr;
            delete timer;
            return;
        }


        timer->next->prev = timer->prev;
        timer->prev->next = timer->next;
        delete timer;


    }

    //每次来SIGALARM就触发tick函数
    void tick()
    {
        if (!head)
        {
            return;
        }

        printf("TICK! TICK!\n");
        time_t cur = time(0);

        Timer* tmp = head;

        while (tmp)
        {
            if (cur < tmp->expire) //未超时
            {
                break;
            }
            //超时执行回调
            tmp->time_cb(tmp->user_data);
            head = tmp->next;
            if (head)
            {
                head->prev = nullptr;
            }
            delete tmp;
            tmp = head;
        }



    }



private:
    void Add_Timer_After(Timer* timer, Timer* list_head)
    {
        Timer* prev = list_head;
        Timer* tmp = prev->next;
        
        while (tmp)
        {
            if (timer->expire < tmp->expire)
            {
                prev->next = timer;
                tmp->prev = timer;
                timer->prev = prev;
                timer->next = tmp;
                return;
            }

            prev = tmp;
            tmp = tmp->next;
        }

        if (!tmp)
        {
            tail->next = timer;
            timer->prev = tail;
            tail = timer;
            return;
        }        

    }


    Timer* head;
    Timer* tail;
};



