## 1. Prefer task-based programming to thread-based

- 如果希望异步地运行一个函数
    - 基于线程的做法

        ```c++
        int doAsyncWork();
        std::thread t(doAsyncWork);
        ```
    - 基于任务的做法

        ```c++
        auto fut = std::async(doAsyncWork);
        ```
    - 区别是：基于线程的做法没办法访问函数的返回值，或者当出现异常时，程序会直接崩溃；而基于任务的做法能够访问返回值，并且能够返回异常的结果，保证程序不会崩溃
- **C++**并发概念中线程的三个含义
    - **Hardware** **threads**
        - 真正执行计算的线程，每个**CPU**核上面会提供几个这样的硬件线程
    - **Software** **threads**
        - 系统线程，是操作系统管理的所有进程内部的线程，操作系统把它们调度到硬件线程上来执行任务
    - **std::threads**
        - 一个**C++**进程内的对象，是底层软件线程的句柄
- 基于**std::thread**做法的劣势
    - 软件线程是一种有限的资源，当申请超过系统能提供的线程数量时，程序会抛出**std::system\_error**异常，因此程序需要捕获这种异常进行处理，但在用户层难以解决这个问题
    - 当就绪的软件线程数量大于硬件线程数量时，就会出现过载现象，此时，系统的线程调度器按时间片将软件线程调度到硬件线程上执行，而线程切换会给系统增加整体线程管理开销，尤其是当一个硬件线程上的软件线程调度到另一个**CPU**核的硬件线程上时，代价更昂贵，在这种情况下：
        - 新的硬件线程所在的**CPU**缓存对于调度过来的软件线程是冷的，因为没有存储与它有关的数据和指令信息
        - 调度过来的软件线程会给原来的软件线程所在的**CPU**缓存造成污染，因为原来的软件线程可能会再度被调度到这个**CPU**上面
- 从应用开发的角度来避免过载很难，因为这种软件线程和硬件线程的最优比例是瞬息万变的，不仅和硬件设施有关，还和软件行为有关，但是把这份工作交给底层标准库来做会容易很多。
- **std::async**的底层机制
    - 如果当前申请的线程已经超过系统能够提供的线程数量时，调用**std::thread**和**std::async**有什么区别呢？
        - 调用**std::async**并不保证会创建一个新的软件线程，而是它允许调度器把新线程要执行的函数放在当前线程上运行，当前线程是请求新线程并等待执行结果的线程，那么当系统过载或者线程资源不够时，合理的调度器会利用自由方式来解决这些问题。
        - 但是对于**GUI**线程的响应可能会出现问题，因为调度器并不知道应用哪个线程会有高响应度的要求，这时需要对**std::async**使用**std::launch::async**的启动策略，它能确保函数会在另一个不同的线程上执行。
    - 最新的线程调度器会使用系统返回的线程池来避免过载，通过工作窃取来改善所有硬件核的负载均衡。**C++**标准并没有要求这些特性，但是生产商会在标准库中使用这些技术。如果在并发编程中使用基于任务的方法，我们能享受这种技术的好处。
- **std::thread**的使用场景
    - 需要访问底层线程实现的**API**时，**std::thread**能通过**native\_handle()**返回这个句柄
    - 需要优化应用的线程使用时，比如硬件特性和应用的配置文件已知且固定
    - 需要实现一些**C++**并发**API**没有提供的线程技术

## 2. Specify std::launch::async if asynchronicity is essential

- **std::async**有两种异步机制
    - **std::launch::async** 启动机制，这种机制保证函数运行在一个不同的线程中
    - **std::launch::deferred** 启动机制，这种机制使得当**std::async**返回的**future**对象调用了**get**或者**wait**时，异步函数才会被执行。
- 默认情况下，**std::async**的启动机制既不是**std::launch::async**，也不是**std::launch::deferred**，而是它们的并集，即

    ```c++
    auto fut1 = std::async(f);
    ==>等价于
    auto fut2 = std::async(std::launch::async | std::launch::deferred, f);
    ```
    - 因此，默认的启动机制允许函数既可以同步运行，也可以异步运行，这样做是为了允许标准库对线程的管理做优化(处理过载，资源不够，负载不均衡的情况)
- **std::async**的默认启动机制会有一些有趣的含义
    - 无法预测异步函数是否和当前线程并发执行
    - 无法预测异步函数是否执行在新的线程上还是执行在当前线程上
    - 可能也无法预测异步函数是否运行了
    - 以上这些含义使得默认启动机制不能很好地和线程局部变量混用，因为无法预测异步函数所在线程什么时候会执行，也不知道会修改哪些线程局部变量；除此之外，那些使用超时的等待机制循环也会受到影响，因为在一个被延迟的任务上调用**wait\_for**或者**wait\_unti**会产生**std::future\_status::deferred**的值，也就意味着下面的例子可能会一直运行下去

        ```c++
        using namespace std::literals;

        void f()
        {
            std::this_thread::sleep_for(1s);
        }

        auto fut = std::async(f);
        //函数f有可能一直没有被执行，那么就会一直卡在循环的判断上,这
        //种情况在开发和单元测试中一般不会出现，但是在高压负载下就会出现
        while(fut.wait_for(100ms) != std::future_status::ready)
        {
            ...
        }
        ```

- 一种修正方法是在启动之后，马上检查任务状态是否是**std::future\_status::deferred**，然后做相关处理

    ```c++
    auto fut = std::async(f);

    if( fut.wait_for(0s) == std::future_status::deferred)
    {
        ... // 通过get或者wait来同步调用函数f
    }
    else
    {
        while(fut.wait_for(100ms) != std::future_status::ready)
        {
            ...//并发执行其他任务
        }
        ...
    }
    ```

- 使用默认启动机制的**std::async**时，需要满足以下条件
    - 任务不需要与调用线程并发运行
    - 与线程局部变量的读写无关
    - 要么保证**std::async**的**future**会调用**get**或者**wait**函数，要么也能接受异步任务不会被执行的结果
    - 用到**wait\_for**或者**wait\_until**的代码考虑到延迟任务类型的可能性

## 3. Make std::threads unjoinable on all paths

- 一个**thread**对象如果在析构时仍然是**joinable**的，那么会使得程序终止运行
- 每个**std::thread**对象的状态要么是**joinable**，要么是**unjoinable**
    - **joinable**：底层异步线程正在运行，或者阻塞了，或者等待被调度，或者已经运行结束了，都是**joinable**的线程
    - **unjoinable**：默认构造函数构造的**std::thread**对象，被**std::move**移动过的**thread**对象，已经被**join**过的**thread**对象，已经被**detach**过的**thread**对象，都是**unjoinable**的线程
- 要保证在析构线程前，所有的路径上**std::thread**对象都是**unjoinable**
反例

    ```c++
    constexpr auto tenMillion = 10000000;

    bool doWork(std::function<bool(int)> filter, 
    int maxVal = tenMillion)
    {
        std::vector<int> goodVals;
        std::thread t( [&filter, maxVal, &goodVals] {
        for(auto i = 0; i <= maxVal ; ++i)
        {
            if ( filter(i))
                goodVals.push_back(i);
        }
        });
        
        auto nh = t.native_handle();
        ...
        
        if(conditionAreSatisfied()) {
        t.join();
        performComputation(goodVals);
        return true;
        }
        return false; // thread对象没有被join!!!
    }
    ```

- 为什么**std::thread**的析构函数会在线程是**joinable**状态时应该导致程序异常
    - 对于**joinable**的线程，析构时析构函数在等待底层的线程完成，那么会导致行为异常，很难追踪，因为明明**conditionAreSatisfied()**返回**false**，就说明**filter**函数不应该在执行中了，而析构函数等待这意味着上层的**filter**函数应该在继续执行。
    - 对于**joinable**的线程，析构时析构函数通过**detach**断开了**std::thread**对象和底层执行线程的连接后，底层的线程仍然在运行，此时**thread**所在的函数占用的内存已经回收，如果后面仍有函数调用的话，那么新函数将会使用这片内存，而此时如果底层线程修改了原来函数的内存空间时，新函数占用的内存就会被修改!!!
- 推荐的做法是使用一种**RAII**对象来将**thread**对象包含在内，使用**RAII**对象来保证**thread**资源的**join**

    ```c++
    class ThreadRAII {
        public:
            enum class DtorAction { join, detach };
            ThreadRAII(std::thread&& t, DtorAction a):
            action(a), t(std::move(t)) {}
            ~ThreadRAII()
            {
                if(t.joinable()) {
                    if (action == DtorAction::join){
                    t.join();
                    } else {
                    t.detach();
                    }
                }
            }
        
            ThreadRAII(ThreadRAII&&) = default;
            ThreadRAII& operator=(ThreadRAII&&) = default;
            std::thread& get() { return t;}
        private:
            DtorAction action;
            std::thread t;
    };
    ```

## 4. Be aware of varying thread handle destructor behavior

- 一个**non-deferred**的**std::future**和一个**joinable**的**std::thread**都可以看做是系统线程的句柄，但是它们的析构函数行为不同
    - 一个**joinable**的**std::thread**对象进行析构时，会引发程序终止运行
    - 一个**non-deferred**的**std::future**对象进行析构时，有时候看起来像执行了隐式的**join**，有时候看起来像执行了隐式的**detach**，有时候又都不像，但是它从来不会引起程序终止运行
- 为什么一个**non-deferred**的**std::future**对象会有这样的行为?
    - 因为调用者的**future**和被调用者的**promise**在传递结果时，这个结果既没有存放在**promise**，也没有存放在**future**，而是存放在一个堆对象代表的**shared** **state**中，而标准没有指定个对象的行为，因此标准库厂商可以以任意方式来实现这个对象。
    - **std::future**的析构函数会呈现出哪一种行为，取决于**shared** **state**对象的实现方式
        - 异常行为：对于通过**std::async**启动的**non-deferred**任务，它的**shared** **state**对象如果被最后一个**future**引用，那么这个**future**在析构时就会阻塞
            - 触发条件：
                - **future**引用一个由**std::async**产生的**shared** **state**对象
                - 任务的启动策略是**std::launch::async**
                - 这个**future**是引用**shared** **state**的最后一个**future**
        - 正常行为：其他**future**对象在析构时都只会破坏这个**future**对象
    - 产生**shared** **state**对象的另一种方式是使用**std::packaged\_task**
        - 一个**std::packaged\_task**对象包含一个异步执行的函数，并将结果存储在一个**shared** **state**对象中
        - 可以通过**std::packaged\_task**的**get\_future**函数可以获得引用内部**shared\_state**对象的**future**对象

            ```c++
            int calcValue();
            std::packaged_task<int()> pt(calcValue);
            // fut会正常析构，因为不满组上面的条件1
            auto fut = pt.get_future(); 
            ```
        - **std::packaged\_task**是不可拷贝的，因此当把它传递给**thread**对象时只能使用**std::move**来转换成右值

            ```c++
            {
                std::packaged_task<int()> pt(calcValue);
                auto fut = pt.get_future();
                std::thread t(std::move(pt));
                ...
            }
            ```
            - 上面的代码在...处有三种可能情况
                - **thread**对象**t**没有调用**join**或者**detach**，这种情况下，程序会崩溃
                - **thread**对象**t**调用了**join**，这种情况下**fut**对象在析构时不会阻塞
                - **thread**对象**t**调用了**detach**，这种情况下**fut**对象在析构时不需要**detach**了
            - 也就是说，由**std::packaged\_task**产生的**fut**不需要采用特定的析构策略

## 5. Consider void futures for one-shot event communication

- 有时一个任务需要告诉第二个异步运行的任务某个特定的事件是否已经发生了，因为第二个任务需要等待事件发生才能继续进行下去，此时线程间通信的最佳方式是什么呢？
    - 条件变量方式

        ```c++
        std::condition_variable cv;
        std::mutex m；
        // task 1
        {
            ...   // detect event        
            cv.notify_one();
        }

        //task 2
        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk);
            ... // react to event
        }
        ```
        - 问题1：如果**task1**在**task2**调用**wait**之前调用了**notify\_one**，那么**task2**就会一直挂起
        - 问题2：**wait**函数没有考虑到错误的唤醒，此时**task1**尚未调用**notify\_one**
    - 共享布尔变量

        ```c++
        std::atomic<bool> flag(false);
        //task 1
        {
            ...  // detect event
            flag = true;
        }

        //task 2
        {
            ...
            while(!flag); // wait for event
            ...
        }
        ```
        - 问题是**while**循环空转会浪费**CPU**资源
    - 条件变量加布尔变量组合

        ```c++
        std::condition_variable cv;
        std::mutex m;
        bool flag(false);

        //task 1
        {
            ... // detect event
            {
            std::lock_guard<std::mutex> g(m);
            flag = true;
            }
            cv.notify_one();
        }

        //task 2
        {
            ...
            {
            std::unique_lock<std::mutex> lk(m);
            // use lambda to avoid spurious wakeups
            cv.wait(lk, [] {return flag; }); 
            ...  // react to event
            }
            ...
        }
        ```
    - **future+promise**方式

        ```c++
        std::promise<void> p;

        // task 1
        {
            ... // detect event
            p.set_value(); // tell reacting task
        }

        // task 2
        {
            ...
            // wait on future corresponding to p
            p.get_future().wait(); 
            ... // react to event
        }
        ```
        - 缺点是**std::promise**只能被设置一次，不能被重复使用
        - 这种方式还可以用来启动一个**suspend**的线程

            ```c++
            std::promise<void> p;
            void react();
            void detect()
            {
                std::thread t([] {
                    p.get_future().wait(); 
                    react();
                });
                ... // do something else before starting thread
                p.set_value();
                ...
                t.join();
            }
            ```
        - 或者控制多个线程的启动

            ```c++
            std::promise<void> p;

            void detect()
            {
                auto sf = p.get_future().share();
                for(int i = 0; i < threadsToRun; ++i)
                {
                    vt.emplace_back([sf] { sf.wait(); react(); });
                }
                ...
                p.set_value();
                ...
                for(auto& t : vt){
                    t.join();
                }
            }
            ```

## 6. Use std::atomic for concurrency, volatile for special memory

- **std::atomic**使得多线程并发访问的顺序得到控制
- **std::volatile**使得编译器不会优化这类变量的代码，因为有些代码在原本的优化规则里面是允许的，但是在逻辑上是不允许进行优化的

## 7. Summary

- **The std::thread API offers no direct way to get return values from asynchronously run functions, and if those functions throw, the program is terminated.**
- **Thread-based programming calls for manual management of thread exhaustion, oversubscription, load balancing, and adaptation to new platforms.**
- **Task-based programming via std::async with the default launch policy handles most of these issues for you.**
- **The default launch policy for std::async permits both asynchronous and synchronous task execution.**
- **This flexibility leads to uncertainty when accessing thread\_locals, implies that the task may never execute, and affects program logic for timeout-based wait calls.**
- **Specify std::launch::async if asynchronous task execution is essential.**
- **Make std::threads unjoinable on all paths.**
- **Join-on-destruction can lead to difficult-to-debug performance anomalies.**
- **Detach-on-destruction can lead to difficult-to-debug undefined behavior.**
- **Declare std::thread objects last in lists of data members.**
- **Future destructors normally just destroy the future's data members.**
- **The final future referring to a shared state for non-deferred task launched via std::async blocks until the task completes.**
- **For simple event communication, condvar-based desgins require a superfluous mutex, impose constraints on the relative progress of detecting and reacting tasks, and require reacting tasks to verify that the event has taken place.**
- **Designs employing a flag avoid those problems, but are based on polling, not blocking.**
- **A condvar and flag can be used together, but the resulting communications mechanism is somewhat stilted.**
- **Using std::promises and futures dodges these issues, but the approach uses head memory for shared states, and it's limited to one-shot communication.**
- **std::atomic is for data accessed from multiple threads without using mutexes. It's a tool for writing concurrent software.**
- **volatile is for memory where reads and writes should not be optimized away. It's a tool for working with special memory.**
