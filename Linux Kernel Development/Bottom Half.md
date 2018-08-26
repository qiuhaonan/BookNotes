### 6.3.1 使用tasklets
- 声明tasklet
    - 静态创建
        - DECLARE_TASKLET(name, func, data);
        - DECLARE_TASKLET_DISABLED(name, func, data); //将引用计数器设置为1，禁用此tasklet
    - 动态创建
        - tasklet_init(t, tasklet_handler, dev);
- 编写tasklet处理程序
    - void tasklet_handler(unsigned long data);
    - 因为靠软中断实现，因此不能睡眠，可以响应中断，要做好并发访问的保护工作
- 调度tasklet
    - tasklet_schedule(&my_tasklet); //把my_tasklet标记为挂起
    - 在tasklet被调度以后，只要有机会就会尽早运行。在它还没得到运行机会之前，如果有一个相同的tasklet又被调度了，那么它仍然只会运行一次；而如果这时它已经开始运行了，那么这个新的tasklet会被重新调度并再次运行
- 禁用tasklet
    - tasklet_disable()
    - 如果该tasklet正在执行，这个函数会等到它执行完毕再返回
- 去掉tasklet
    - tasklet_kill()
    - 从挂起的队列中去掉一个tasklet
    - 首先等待该tasklet执行完毕，然后再将它移去

### 6.3.2 ksoftirqd
每个处理器都有一组辅助处理软中断和tasklet的内核线程，当内核中出现大量软中断的时候，这些内核线程就会辅助处理它们。

如果软中断被触发的频率很高(像在进行大流量的网络通信期间)，再加上它们又有将自己重新设置为可执行状态的能力，那么就会导致用户空间进程无法获得足够的处理器时间，因而也就处于饥饿状态。直观的解决方法有：
- 只要还有被触发并等待处理的软中断，本次执行就要负责处理，重新触发的软中断也在本次执行返回前被处理。
    - 负载很高的时候，系统可能会一直处理软中断，用户空间的任务被忽略了
- 选择不处理重新触发的软中断
    - 任何自行重新触发的软中断都不会马上处理，它们被放到下一个软中断执行时机去处理，而这个时机通常也就是下一次中断返回的时候。
    - 可是，在比较空闲的系统中，立即处理软中断才是比较好的做法

内核实际选中的方案是不会立即处理重新触发的软中断，而作为改进，当大量软中断出现的时候，内核会唤醒一组内核线程来处理这些负载，这些线程在最低的优先级上运行。实际工作流程如下：
```c
for(;;)
{
    if( !softirq_pending( cpu ) )
        schedule();
    
    set_current_state( TASK_RUNNING );

    while( softirq_pending( cpu) )
    {
        do_softirq();
        if( need_resched() )
            schedule();
    }

    set_current_state( TASK_INTERRUPTIBLE );
}
```
只要do_softirq()函数发现已经执行过的内核线程重新触发了它自己，软中断内核线程就会被唤醒

### 6.4 工作队列
工作队列是另外一种将工作推后执行的形式。工作队列可以把工作推后，交由一个内核线程去执行，该工作总是会在进程上下文执行。这样就允许重新调度甚至是睡眠。

工作者线程用workqueue_struct结构表示：
```c
struct workqueue_struct 
{
    struct cpu_workqueue_struct cpu_wq[NR_CPUS];
};
```

每个工作者线程对应一个cpu_workqueue_struct结构体：
```c
struct cpu_workqueue_struct
{
    spinlock_t lock;

    atomic_t nr_queued;
    struct list_head worklist;
    wait_queue_head_t more_work;
    wait_queue_head_t work_done;

    struct workqueue_struct *wq;
    task_t* thread;
    struct completion exit;
};
```
每个工作者线程类型关联一个自己的workqueue_struct

工作用<linux/workqueue.h>中定义的work_struct结构体表示：
```c
struct work_struct
{
    unsigned long pending;  //该工作是否等待处理
    struct list_head entry; //连接所有工作的链表
    void (*func) (void*);   //处理函数
    void *data;             //传递给处理函数的参数
    void *wq_data;          //内部使用
    struct timer_list timer;//延迟工作队列所用到的定时器
};
```

工作者线程都要执行worker_thread()函数，核心流程如下：
```c
for(;;)
{
    set_task_state(current, TASK_INTERRUPTIBLE); //设置成休眠状态
    add_wait_queue(&cwq->more_work, &wait);      //加入到等待队列上

    if(list_empty(&cwq->worklist))               //如果工作链表是空的，就进入睡眠状态
        schedule();
    else
        set_task_state(current, TASK_RUNNING);   //否则设置为运行状态
    remove_wait_queue(&cwq->more_work, &wait);   //脱离等待队列

    if(!list_empty(&cwq->worklist))              //执行被推后的工作
        run_workqueue(cwq);
}
```
其中run_workqueue()函数实际流程如下：
```c
while(!list_empty(&cwq->worklist))
{
    struct work_struct *work = list_entry(cwq->worklist.next, struct work_struct, entry);
    void (*f) (void *) = work->func;
    void *data = work->data;

    list_del_init(cwq->worklist.next);
    clear_bit(0, &work->pending);
    f(data); 
}
```

## 6.6 在下半部之间加锁
在使用下半部机制时，即使是在一个单处理器的系统上，避免共享数据被同时访问也是至关重要的，因为一个下半部实际上可能在任何时候执行。

使用tasklets的一个好处在于它自己负责执行的序列化保障：两个相同类型的tasklet不能同时执行，即使在不同的处理器上也不行。而软中断根本不保障执行序列化，即使相同类型的软中断也有可能有两个实例在同时执行，所以所有的共享数据都需要合适的锁。

如果进程上下文和一个下半部共享数据，在访问这些数据之前，你需要禁止下半部的处理并得到锁的使用权。所做的这些都是为了本地和SMP的保护并且防止死锁的出现。

如果中断上下文和一个下半部共享数据，在访问数据之前，你需要禁止中断并得到锁的使用权。所做的这些也是为了本地和SMP的保护并且防止死锁的出现。

任何在工作队列中被共享的数据也需要使用锁机制。
