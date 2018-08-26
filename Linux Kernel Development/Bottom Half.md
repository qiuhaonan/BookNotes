# 6. 下半部和推后执行的工作
中断处理程序本身的局限使得它只能完成整个中断处理流程的上半部分，这些局限包括：
- 中断处理程序以异步的方式执行并且它有可能会打断其他重要代码(甚至包括其他中断处理程序)的执行，因此，中断处理程序应该执行的越快越好
- 中断处理程序在执行时，在最好的情况下，与该中断同级的其他中断会被屏蔽，在最坏的情况下，所有其他中断都会被屏蔽，因此，中断处理程序应该执行的越快越好
- 由于中断处理程序往往需要对硬件进行操作，所以它们通常有很高的时限要求
- 中断处理程序不再进程上下文中运行，所以不能阻塞

总的来说就是，需要一个快速、异步、简单的处理程序负责对硬件做出迅速响应并完成那些时间要求很严格的操作。

但是，对于那些其他的、对时间要求相对宽松的任务，就应该推后到中断被激活以后再去运行。

## 6.1 下半部
下半部的任务就是执行与中断处理密切相关但中断处理程序本身不执行的工作。理想情况是所有工作都交给下半部执行，但中断处理程序注定要完成一部分工作，例如：通过操作硬件对中断的到达进行确认，有时还会从硬件拷贝数据。

遗憾的是，并不存在严格明确的规定来说明到底什么任务应该在哪个部分中完成。但还是有一些提示可供借鉴：
- 如果一个任务对时间非常敏感， 将其放在中断处理程序中执行
- 如果一个任务和硬件相关， 将其放在中断处理程序中执行
- 如果一个任务要保证不被其他中断打断， 将其放在中断处理程序中执行
- 其他所有任务， 考虑放置在下半部执行

## 6.2 为什么要使用下半部
因为中断处理程序运行时会屏蔽同类型甚至所有本地中断，而缩短中断被屏蔽的时间对系统的响应能力和性能都至关重要，再加上中断处理程序要与其他程序(甚至是其他的中断处理程序)异步执行，因此，为了缩短中断处理程序的执行，就必须把一些工作放到下半部去做

具体放到以后的什么时候去做呢？下半部并不需要指明一个确切时间，只要把这些任务推迟一点，让它们在系统不太繁忙并且中断恢复后执行就可以了。通常下半部在中断处理程序一返回就会马上执行。下半部执行的关键在于当它们运行的时候，允许响应所有的中断

## 6.3 下半部的环境
在linux中，由于上半部从来都只能通过中断处理程序实现，所以它和中断处理程序可以说是等价的，但下半部可以通过多种机制实现。
- BH
    - 一个静态创建、由32个bottom half组成的链表
    - 上半部通过一个32位整数中的一位来标识出哪个bottom half可以执行
    - 每个BH都在全局范围内进行同步，即使分属于不同的处理器，也不允许任何两个bottom half同时执行
    - 方便但不灵活，简单却有性能瓶颈
- 任务队列(task queues)
    - 内核定义一组任务队列
    - 每个队列都包含一个由等待调用的函数组成链表
    - 根据其所处队列的位置，这些函数会在某个时刻被执行
    - 驱动程序可以把它们自己的下半部注册到合适的队列上去
    - 不灵活，不能胜任性能要求高的子系统
- 软中断(softirqs,与系统调用的软中断是不同的概念)
    - 软中断是一组静态定义的下半部接口(编译器就静态注册)，有32个，可以再所有的处理器上同时执行
- tasklet
    - tasklet是一种基于软中断实现的灵活性强、动态创建的下半部机制，两个不同类型的tasklets可以在不同的处理器上同时执行
    - tasklet是一种在性能和易用性之间寻求平衡的产品
    - 对于大部分下半部处理来说，用tasklet就足够了，像网络这样对性能要求非常高的情况才使用软中断
- 工作队列(work queues)
    - 

## 6.4 软中断
软中断是在编译期间静态分配的，由softirq_action结构表示，定义在<linux/interrupt.h>中：
```c
struct softirq_action
{
    void (*action) (struct softirq_action *); // 函数
    void *data; // 函数参数
};
```
kernel/softirq.c中定义了一个包含有32个该结构体的数组：
```c
static struct softirq_action softirq_vec[32];
```
每个被注册的软中断都占据该数组的一项，因此最多可能有32个软中断。一个软中断不会抢占另一个软中断。实际上，唯一可以抢占软中断的是中断处理程序。

一个注册的软中断必须在被标记后才会执行，称作触发软中断(raising the softirq)。通常，中断处理程序会在返回前标记它的软中断，使其在稍后被执行。待处理的软中断在下列地方会被检查和执行：
- 在处理完一个硬件中断以后
- 在ksoftirq内核线程中
- 在那些显式检查和执行待处理的软中断的代码中，如：网络子系统中

软中断一律在do_softirq()中执行：
```c
u32 pending = softirq_pending(cpu);

if(pending)
{
    struct softirq_action* h = softirq_vec;
    softirq_pending(cpu) = 0;
    do
    {
        if(pending & 1)
            h->action(h);
        h++;
        pending >> = 1;
    }
    while(pending);
}
```
软中断保留给系统中对时间要求最严格以及最重要的下半部使用。使用流程：
- 分配索引
    - 在编译期间在<linux/interrupt.h>中定义的一个枚举类型来静态的声明软中断
- 注册软中断处理程序
    - 运行时通过调用open_softirq()注册软中断处理程序，函数有三个参数：软中断索引号，处理函数和data域存放的数值
    - 软中断处理程序执行的时候，允许响应中断，但自己不能休眠且当前处理器上的软中断被禁止
- 触发软中断
    - 通过raise_softirq()函数将一个软中断设置为挂起状态，让它在下次调用do_softirq()函数时投入运行。该函数在触发一个软中断之前先要禁止中断，触发后再恢复回原来的状态。
例如：
```c
open_softirq(NET_TX_SOFTIRQ, net_tx_action, NULL);
open_softirq(NET_RX_SOFTIRQ, net_rx_action, NULL);
...
raise_softirq(NET_TX_SOFTIRQ);
```

## 6.5 Tasklets
Tasklets由tasklet_struct结构体表示，每个结构体单独代表一个tasklet，在<linux/interrupt.h>中定义：
```c
struct tasklet_struct 
{
    struct tasklet_struct *next; //链表中下一个结构体
    unsigned long state; //tasklet的状态: 0， TASKLET_STATE_SCHED, TASKLET_STATE_RUN
    atomic_t count; //引用计数器, 仅当为0时才被激活
    void (*func) (unsigned long); //tasklet处理函数
    unsigned long data; //给tasklet处理函数的参数
};
```

已调度的tasklet存放在两个单处理器数据结构: tasklet_vec和tasklet_hi_vec中，都是由tasklet_struct结构体构成的链表。

Tasklets由tasklet_schedule()和tasklet_hi_schedule()调度：
- 检查tasklet的状态是否为TASKLET_STATE_SCHED。如果是，函数返回。
- 保存中断状态，然后禁止本地中断。(在执行tasklet代码时，保证处理器上的数据不会弄乱)
- 把需要调度的tasklet加到每个处理器一个的tasklet_vec链表或tasklet_hi_vec链表的表头上去
- 唤起TASKLET_SOFTIRQ或HI_SOFTIRQ软中断，这样在下一次调用do_softirq()时就会执行该tasklet
- 恢复中断到原状态并返回

由于大部分tasklets和软中断都是在中断处理程序中被设置成待处理状态，所以最近一个中断返回的时候看起来就是执行do_softirq()的最佳时机。因为TASKLET_SOFTIRQ和HI_SOFTIRQ已经被触发了，所以do_softirq()会执行相应的软中断处理程序tasklet_action()：
- 禁止中断，并为当前处理器检索tasklet_vec或tasklet_hi_vec链表
- 将当前处理器上的该链表设置为NULL，达到清空的效果
- 允许响应中断
- 循环遍历获得链表上的每一个待处理的tasklet
- 如果是多处理器系统，通过检查TASKLET_STATE_RUN来判断这个tasklet是否正在其他处理器上运行，如果是，就跳到下一个待处理的tasklet去，否则就将其状态设置为TASKLET_STATE_RUN，禁止别的处理器来执行它
- 检查count值是否为0，确保tasklet没有被禁止，执行tasklet处理程序，否则跳到下一个挂起的tasklet去
- tasklet运行完毕，清除tasklet的state域
- 重复执行下一个tasklet，直到没有剩余的等待处理的tasklets


### 6.5.1 使用tasklets
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

### 6.6 ksoftirqd
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

### 6.7 工作队列
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

## 6.8 在下半部之间加锁
在使用下半部机制时，即使是在一个单处理器的系统上，避免共享数据被同时访问也是至关重要的，因为一个下半部实际上可能在任何时候执行。

使用tasklets的一个好处在于它自己负责执行的序列化保障：两个相同类型的tasklet不能同时执行，即使在不同的处理器上也不行。而软中断根本不保障执行序列化，即使相同类型的软中断也有可能有两个实例在同时执行，所以所有的共享数据都需要合适的锁。

如果进程上下文和一个下半部共享数据，在访问这些数据之前，你需要禁止下半部的处理并得到锁的使用权。所做的这些都是为了本地和SMP的保护并且防止死锁的出现。

如果中断上下文和一个下半部共享数据，在访问数据之前，你需要禁止中断并得到锁的使用权。所做的这些也是为了本地和SMP的保护并且防止死锁的出现。

任何在工作队列中被共享的数据也需要使用锁机制。
