# Effective Modern C++(11&14)Chapter4: Smart Pointers

## 1. Introduction

- 原始指针 **(raw pointer) p** 的缺点
    - **p** 的声明不能暗示 **p** 指向的是单个对象还是一个数组
    - **p** 的声明不能暗示在使用完 **p** 后是否应该销毁 **p**
    - 如果使用完 **p** 后决定销毁 **p**，无法知道是该使用 **delete** 还是其他析构机制来销毁 **p**
    - 如果是使用 **delete** 来销毁 **p**，无法知道是该使用 **delete** 还是 **delete[]** 来销毁 **p**
即便知道了具体的销毁方式，很难保证在所有的代码路径上只销毁了一次 **p**，少一次会造成内存泄露，多一次会造成未定义行为
    - 通常无法对 **p** 判断其是否是悬空指针
- **C++11** 中的四种智能指针
    - **std::auto\_ptr** (在 **C++98** 以后被 **std::unique\_ptr** 取代了)
    - **std::unique\_ptr**
    - **std::shared\_ptr**
    - **std::weak\_ptr**

## 2. Use std::unique\_ptr for exclusive-ownership resource management

- 默认情况下(不传入自定义析构器时)， **std::unique\_ptr** 和原始指针大小一样，对于大多数的操作，它们执行相同的指令，也就是说在内存和计算资源吃紧的地方也可以用 **std::unique\_ptr**
- **std::unique\_ptr** 呈现出来的是独占使用权语义，因此， **std::unqiue\_ptr** 不能拷贝，只能移动，析构时非空的 **std::unique\_ptr** 会销毁它的资源，默认情况下， **std::unique\_ptr** 会对内部的原始指针使用 **delete** 来释放原始指针所指向的资源。
- 通用的例子是将 **std::unique\_ptr** 作为返回层次结构中对象的工厂函数的返回类型，对于这样一个层次结构，工厂函数通常在堆上分配一个对象，然后返回指向该对象的指针，而工厂函数调用者则负责在使用完对象后，对对象堆内存的释放
    ```c++
    class Investment {...};
    class Stock: public Investment {...};
    class Bond: public Investment {...};
    class RealEstate: public Investment {...};
    template<typename... Ts>
    std::unique_ptr<Investment> makeInvestment(Ts&&... params);
    //using
    {
    ...
    auto pInvestment = makeInvestment(arguments);
    ...
    }
    ```

- **std::unique\_ptr** 也可以用在使用权迁移的场景，比如，当从工厂函数返回的 **std::unique\_ptr** 被移动到一个容器中，而这个容器后来又被移动到一个对象的数据成员中。当这个对象销毁时， **std::unique\_ptr** 管理的资源也会自动销毁。如果使用权链受到异常或其他非典型控制流中断， **std::unique\_ptr** 管理的资源最终也会被释放，仅仅在三种条件下不会释放：
    - 异常传播到线程主函数之外
    - 异常出现在声明了 **noexcept** 的地方，局部对象也许不会被销毁
    - 调用了 **std::abort，std::\_Exit，std::exit** 或者 **std::quick\_exit** 函数，对象一定不会被销毁
- 给 **std::unique\_ptr** 传入自定义析构器

    ```c++
    auto delInvmt = [](Investment* pInvestment){
        //此处使用基类指针来释放对象，为了正确释放实际对象类型的内存，
        需要将基类的析构函数设为虚函数
        makeLogEntry(pInvestment);
        delete pInvestment;
    };
    template<typename... Ts>
    std::unique_ptr<Investment, decltype<delInvmt>
    makeInvestment(Ts&&... params)
    {
        std::unique_ptr<Investment, decltype(delInvmt)>
        pInv(nullptr, delInvmt);

        if(...)
            pInv.reset(new Stock(std::forward<Ts>(params)...));
        else if(...)
            pInv.reset(new Bond(std::forward<Ts>(params)...));
        else if(...)
            pInv.reset(new RealEstate(std::forward<Ts>(params)...));

        return pInv;
    }
    ```

- 在对 **std::unique\_ptr** 设置自定义析构器后， **std::unique\_ptr** 的大小不再等于原始指针的大小
    - 当自定义析构器是函数指针时， **std::unique\_ptr** 的大小从 **1** 个字长变为 **2** 个字长
    - 当自定义析构器是函数对象时， **std::unique\_ptr** 的大小取决于函数对象内部存储多少状态，无状态函数对象(例如：无捕捉的 **lambda** 表达式)不会增加 **std::unique\_ptr** 的尺寸，因此，当函数指针和无捕捉的 **lambda** 对象都可用时，使用无捕捉的 **lambda** 对象更好
        ```c++
        auto delInvmt1 = [](Invest* pInvestment){
                makeLogEntry(pInvestment);
                delete pInvestment;
        };

        template<typename... Ts>
        std::unique_ptr<Investment, decltype(delInvmt1)>
        makeInvestment(Ts&&... args);
        // return type has size of Investment*
        void delInvmt2(Investment* pInvestment)
        {
                makeLogEntry(pInvestment);
                delete pInvestment;
        }

        template<typename... Ts>
        std::unique_ptr<Investment, void (* )(Investment*)>
        makeInvestment(Ts&&... params);
        //return type has sizeof(Investment*)+least sizeof(function pointer)
        ```

- **std::unique\_ptr** 有两种形式，一种是针对单个对象( **std::unique\_ptr<T>** )，另一种是针对数组( **std::unique\_ptr<T[]>** )，针对单个对象时，不能使用  **\*** 运算，而针对数组对象时不能使用 **\***  和 **->** 运算
- **std::unique\_ptr** 可以转换到 **std::shared\_ptr** ，但是反过来不可以  

## 3. Use std::shared\_ptr for shared-ownership resource management

- **std::shared\_ptr** 是 **C++11** 中做到可预测的自动资源管理方式，具有和垃圾回收一样的自动资源管理，但时间可预测，而不是由垃圾回收器那种决定哪些内存在什么时候回收
- 一个通过 **std::shared\_ptr** 访问的对象，它的生命周期由这些指针通过共享使用权来管理，没有特定的 **std::shared\_ptr** 拥有这个对象，而是所有指针一起协作来确保在对象使用完后，销毁这个对象。当最后一个指向对象的 **std::shared\_ptr** 不再指向该对象时， **std::shared\_ptr** 就会销毁这个对象，因此这个销毁的时间是可以确定的
- 一个 **std::shared\_ptr** 通过查询和持有对象 **a** 相关的引用计数，来判断它是不是最后一个指向该对象 **a** 的智能指针，这个引用计数追踪有多少个 **std::shared\_ptr** 在指向对象 **a** ，每构造一个指向 **a** 的 **std::shared\_ptr** ，这个引用计数就加 **1** (通常情况)，每析构一个指向 **a** 的 **std::shared\_ptr** ，这个引用计数就减 **1** ，拷贝赋值时，两者都会执行(指针 **a** 和 **b** 指向两个不同的对象，那么 **a = b** 就会对 **a** 指向对象的引用计数减 **1** ，对 **b** 指向对象的引用计数加 **1** )
- 引用计数的存在有一些性能影响
    -  **std::shared\_ptr** 的大小是原始指针大小的两倍
    - 引用计数的内存必须是动态分配的，因为被引用对象本身不知道引用计数的存在，被引用对象也就没有地方保存这个计数；另外如果使用 **make\_shared** 来构造 **std::shared\_ptr** ，则可以省去这次动态内存分配
    - 对引用计数的修改必须是原子操作，因为多个使用者可能并发读写该引用计数
- 构造 **std::shared\_ptr** 在移动构造情况下，不会对引用计数进行修改
-  **std::shared\_ptr** 的自定义析构器和 **std::unique\_ptr** 自定义的析构器区别
    - 对于 **std::unique\_ptr** ，自定义析构器属于 **std::unique\_ptr** 的一部分
    - 对于 **std::shared\_ptr** ，自定义析构器不属于 **std::shared\_ptr** 的一部分
        ```c++
        auto loggingDel = [](Widget* pw) {
            makeLogEntry(pw);
            delete pw;
        };

        //自定义析构器是指针的一部分
        std::unique_ptr<Widget, decltype(loggingDel)> upw(new Widget, loggingDel);
        //自定义析构器不是指针的一部分
        std::shared_ptr<Widget> spw(new Widget, loggingDel);

        auto customDeleter1 = [](Widget* pw){...};
        auto customDeleter2 = [](Widget* pw){...};

        std::shared_ptr<Widget> pw1(new Widget, customDeleter1);
        std::shared_ptr<Widget> pw2(new Widget, customDeleter2);
        //带有不同自定义析构器的同类型std::shared_ptr可以被放在同一个容器中
        std::vector<std::shared_ptr<Widget>> vpw{pw1, pw2};
        ```

- 自定义析构器可以是函数指针，函数对象， **lambda** 表达式，但是 **std::shared\_ptr** 的大小仍然不变，为什么?
    - 因为这些自定义析构器的内存和 **std::shared\_ptr** 内存不是同一片内存
    - 更具体的说， **std::shared\_ptr** 包含的是一个指向对象的指针和一个指向控制块的指针，而这个控制块里面包含引用计数，弱指针计数，自定义析构器，自定义分配器，虚函数等等
    - 一个对象的控制块是由创建第一个指向该对象的 **std::shared\_ptr** 的函数设定的，而一般来说创建 **std::shared\_ptr** 的函数不可能知道是否已经有其他 **std::shared\_ptr** 指向了该对象，因此需要设定如下规则：
        -  **std::make\_shared** 函数总是创建一个控制块
        - 用一个独占使用权的指针(例如： **std::unique\_ptr** 和 **std::auto\_ptr** )来构造一个 **std::shared\_ptr** 时，需要创建一个控制块
        - 用一个原始指针来构造一个 **std::shared\_ptr** 时，需要创建一个控制块
    - 以上规则暗示了：如果使用一个原始指针分别构造了多个 **std::shared\_ptr** ，那么就会出现多个独立的控制块，也会造成多次资源释放
        ```c++
        auto pw = new Widget;
        ...
        std::shared_ptr<Widget> spw1 (pw, loggingDel);
        ...
        std::shared_ptr<Widget> spw2 (pw, loggingDel);
        ```

- 第二次资源释放时会造成未定义行为
    - 因此，有两个经验需要知道
      - 尽量避免使用原始指针来构造 **std::shared\_ptr**
      - 如果要使用原始指针来构造 **std::shared\_ptr** ，那么最好在 **new** 之后就将指针传给 **std::shared\_ptr** 的构造函数，然后使用现有的 **std::shared\_ptr** 来复制构造其他的 **std::shared\_ptr**

        ```c++
        std::shared_ptr<Widget> spw1(new Widget, loggingDel);
        std::shared_ptr<Widget> spw2(spw1);
        ```

- 如果使用 **this** 指针构造多个 **std::shared\_ptr** ，也会创造多个控制块，当外部有其他 **std::shared\_ptr** 指向当前 **this** 指针时，就会导致多次释放同一个资源
    ```c++
    std::vector<std::shared_ptr<Widget>> processedWidgets;
    class Widget {
        public:
            ...
            void process();
            ...
    };

    void Widget::process()
    {
        ...
        processWidgets.emplace_back(this);
    }
    ```

- 标准库中解决这个问题的方式是让 **Widget** 类继承自 **std::enable\_shared\_from\_this** 类，并且在使用 **this** 构造 **std::shared\_ptr** 的地方使用 **shared\_from\_this** ()函数代替 **this**
    ```c++
    class Widget: public std::enable_shared_from_this<Widget> {
        public:
            ...
            void process();
            ...
    };

    void Widget::process()
    {
            ...
            processedWidgets.emplace_back(shared_from_this());
    }
    ```

- 在内部， **shared\_from\_this** 会查询当前对象的控制块，然后创建一个新的 **std::shared\_ptr** 来引用该控制块，但是这种做法依赖于当前对象已经有了一个控制块，也就是在调用 **shared\_from\_this** ()的成员函数外部已经有了一个 **std::shared\_ptr** 来指向当前对象，否则的话就是未定义行为。为了防止这种情况，继承自 **std::enable\_shared\_from\_this** 的类通常把构造函数声明为 **private** ，然后通过调用工厂函数来创建对象，并返回 **std::shared\_ptr**

    ```c++
    class Widget: public std::enable_shared_from_this<Widget> {
        public:
            template<typename... Ts>
            static std::shared_ptr<Widget> create(Ts&&... params);
            ...
            void process();
            ...
        private:
            Widget();
            ...
    };        
    ```

- 由 **shared\_ptr** 管理的对象控制块中的虚函数机制通常只会使用一次，那就是在销毁对象的时候
-  **shared\_ptr** 不支持数组管理，因此也就没有  **\*** 运算

## 4. Use std::weak\_ptr for std::shared\_ptr-like pointers that can dangle

- **std::weak\_ptr** 可以表现地像 **std::shared\_ptr** 一样，而且不会影响对象的引用计数，它可以解决 **std::shared\_ptr** 不能解决的问题：引用对象可能已经销毁了
- **std::weak\_ptr** 不能解引用，也不能测试是否是空，因为 **std::weak\_ptr** 不是一个独立的智能指针，而是 **std::shared\_ptr** 的强化版
- **std::weak\_ptr** 通常是从 **std::shared\_ptr** 中创建，它们指向同一个对象， **std::weak\_ptr** 可以通过 **expired** ()来测试指针是否悬空

    ```c++
    auto spw = std::make_shared<Widget>();
    ...
    std::weak_ptr<Widget> wpw(spw);
    ...
    spw = nullptr;
    if(wpw.expired())
        ...
    ```

- 但是通常在测试是否悬空和使用之间可能会出现竞态条件，此时会出现未定义行为，此时需要保证两者作为一体的原子性          std::shared\_ptr<Widget> spw1 = wpw.lock();- 另一种形式是：使用**std::weak\_ptr** 作为**std::shared\_ptr** 构造函数的参数，如果 **std::weak\_ptr** 已经 **expired**，那么就会抛出一个异常
    - 一种形式是：通过 **std::weak\_ptr::lock** 来返回一个**std::shared\_ptr**，如果 **std::weak\_ptr** 已经 **expired** ，那么将会返回一个 **null** 的 **std::shared\_ptr**

        ```c++
        std::shared_ptr<Widget> spw2(wpw);
        ```

-  **std::weak\_ptr** 可以作为缓存来加速查找未失效对象
    - 例如，现在有一个工厂函数基于一个唯一的 **ID** 来产生指向只读对象的智能指针，返回一个 **std::shared\_ptr**

        ```c++
        std::shared_ptr<const Widget> loadWidget(WidgetID id);
        ```

- 如果 **loadWidget** 是一个调用代价较高的函数，一个合理的优化是在内部缓存每次查询的结果，但是每次请求 **Widget** 都要缓存的话会导致性能问题，因此另一个合理的优化是当 **Widgets** 不再需要的时候就从缓存中销毁掉。在这个情况下，调用者从工厂函数中收到智能指针，然后由调用者来决定它的声明周期，而当指向某个 **id** 最后一个使用的指针销毁时，对象也会被销毁，那么缓存中的指针就会悬空，因此在后续查询的时候需要检测命中的指针是否已经悬空，因此，缓存的指针应该是 **std::weak\_ptr**

    ```c++
    std::shared_ptr<const Widget> fastLoadWidget(WidgetID id)
    {
        static std::unordered_map<WidgetID, std::weak_ptr<const widget>> cache;
        auto objPtr = cache[id].lock();
        if(!objPtr){
            objPtr = loadWidget(id);
            cache[id] = objPtr;
        }
        return objPtr;
    }
    ```

- 上面没有考虑累积的失效的 **std::weak\_ptr**
- **std::weak\_ptr** 可用于观察者设计模式中
    - 在这个模式中，对象的状态可能会变化，而观察者需要在对象的状态变化时被提醒，对象在状态变化时提醒观察者很容易，但是它们必须确保观察者没有被销毁，因此一个合理的设计是对象持有观察者的 **std::weak\_ptr** ，使得在访问观察者前可以判断是否还存在
- **std::weak\_ptr** 可用于消除循环引用带来的内存泄露
    - 假设有三个对象 **A, B, C** ，其中 **A** 和 **C** 持有指向 **B** 的 **std::shared\_ptr** ，如果 **B** 也想持有对 **A** 的指针，那么有三种选择
        - 原始指针:如果 **A** 被销毁了，而 **C** 通过 **B** 来访问 **A** 就会出现解引用悬空指针
        - **std::shared\_ptr:** 导致 **A** 和 **C** 的循环引用，当 **A** 和 **C** 都不被程序使用时，各自仍然持有对对方的一个引用计数，因此使得 **A** 和 **C** 无法被释放
        - **std::weak\_ptr:** 完美，当 **A** 被销毁时， **B** 能检测到指向 **A** 的指针已经悬空了，而且能够正确释放 **A** 的内存
- **std::weak\_ptr** 和 **std::shared\_ptr** 大小一样，它们使用相同的控制块和操作，区别仅仅在于 **std::shared\_ptr** 改变的是共享引用计数，而 **std::weak\_ptr** 改变的是弱引用计数

## 5. Prefer std::make\_unique and std::make\_shared to direct use of new

-  **std::make\_shared** 在 **C++11** 中已经存在，而 **std::make\_unique** 则在 **C++14** 中才存在，不过可以手动写一个

    ```c++
    template<typename T, typename... Ts>
    std::unique_ptr<T> make_unique(Ts&&... params)
    {
        return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
    }
    ```

- 有3个 **make** 函数可以接收任意参数集合，把它们完美转发到动态分配对象的构造函数中，然后返回这个对象的只能指针
    - **std::make\_shared**
    - **std::make\_unique**
    - **std::allocate\_shared:** 它表现地和 **std::make\_shared** 一样，除了第一个参数是用于动态内存分配的分配器对象
- 使用 **std::make\_XX** 函数可以减少重复类型的出现

    ```c++
    auto upw1(std::make_unique<Widget>());　//减少了一次Widget的出现
    std::unique_ptr<Widget> upw1(new Widget);

    auto spw2(std::make_shared<Widget>());
    std::shared_ptr<Widget> spw2(new Widget);
    ```

- 使用 **std::make\_XX** 函数可以做到异常安全

    ```c++
    void processWidget(std::shared_ptr<Widget> spw, int priority);
    int computePriority();

    processWidget(std::shared_ptr<Widget>(new Widget), computePriority());
    ```

- 上面的 **processWidget** 调用可能会被编译器优化，最后造成的执行顺序是：**new Widget**, **computePriority**, 构造 **std::shared\_ptr<Widget>**，由于 **new Widget** 对象没有及时保存到 **std::shared\_ptr** 中，结果导致 **new Widget** 内存泄露
- 如果使用 **std::make\_shared** 函数则是

    ```c++
    processWidget(std::make_shared<Widget>(), computePriority());
    ```

- 此时，编译器只能调整两个函数的先后顺序，但是对于 **std::make\_shared** 内部和 **computePriority** 的执行顺序无法优化，因此可以避免动态分配的对象出现内存泄露情况
- **std::make\_XX** 函数可以产生更少更快的代码

    ```c++
    std::shared_ptr<Widget> spw(new Widget);
    auto spw = std::make_shared<Widget>();
    ```

- 使用 **new** 来分配对象并保存到 **std::shared\_ptr** 时，实际上执行了两次动态内存分配，一次为 **Widget** ，另一次为 **std::shared\_ptr** 内部的控制块
- 使用 **std::make\_shared** 函数来实现相同功能时，实际上只执行了一次动态内存分配，一次性为 **Widget** 对象和控制块分配单块内存，同时减少了控制块存储的信息，也减少内存使用量
- **std::make\_XX** 函数的缺点
    - 无法为智能指针传入自定义析构器
    - 内部使用括号进行完美转发参数，如果要使用花括号初始器来构造智能指针，必须直接使用 **new** ，但是完美转发不能直接转发花括号初始化列表，必须先保存为 **std::initializer\_list** 对象，然后在传递给 **std::make\_XX** 函数

        ```c++
        auto initList = {10, 20};
        auto spv = std::make_shared<std::vector<int>>(initList);
        ```

- 对于 **std::make\_unique** 来说，只有上面两中情况会出现问题，而对于 **std::make\_shared** 来说，问题还有很多
- 对于某些自定义 **new** 和 **delete** 的类，它们往往在申请或释放内存时，仅仅申请或释放和对象大小一样的内存，而实际需要的是对象大小加上控制块大小后的内存，因此使用 **std::shared\_ptr** 构造函数不可行，而使用 **std::make\_shared** 函数就无法使用类自定义的 **new** 和 **delete** 运算
- **std::make\_shared** 函数申请的对象内存和控制块内存的生命周期相同，但是控制块还会被 **std::weak\_ptr** 所引用， **std::weak\_ptr** 的 **expired** 函数实际上是对共享引用计数进行检查是否为 **0** ，因此即便为 **0** ，如果弱引用计数不为 **0** ，控制块内存不会被释放，进而对象内存也不会被释放，那么就会造成对象早已不使用，但是仍然被 **std::weak\_ptr** 所引用而造成内存无法释放
- 传入自定义析构器的异常安全问题

    ```c++
    void processWidget(std::shared_ptr<Widget> spw, int priority);
    void cusDel(Widget* ptr);

    processWidget(std::shared_ptr<Widget>(new Widget, cusDel), computePriority());
    // memory leak!!　右值ptr
    ```

- 改进做法

    ```c++
    std::shared_ptr<Widget> spw(new Widget, cusDel);
    processWidget(spw, computePriority()); //spw左值
    ```

- 进一步改进做法，将传入的 **spw** 转换成右值，避免拷贝构造

    ```c++
    processWidget(std::move(spw), computePriority());
    ```

## 6. When using the Pimpl Idiom, define special member functions in the implementation file.

- **Pimpl Idiom** 是一种减少编译量的规则，让每个数据成员转换成类型指针而不是具体的类对象，然后在实现文件中对数据成员指针指向的对象进行动态内存分配和释放

    ```c++
    # widget.h
    class Widget {
        public:
            Widget();
            ~Widget();
            ...
        private:
            struct Impl;
            Impl* pImpl;
    };

    # widget.cpp
    #include"widget.h"
    #include"gadget.h"
    #include<string>
    #include<vector>

    struct Widget::Impl{
        std::string name;
        std::vector<double> data;
        Gadget g1, g2, g3;
    };

    Widget::Widget(): pImpl(new Impl) {}
    Widget::~Widget() {delete pImpl; }
    ```

- 改成智能指针的写法是

    ```c++
    # widget.h
    class Widget {
        public:
            Widget();
            ~Widget();
            ...
        private:
            struct Impl;
            std::unique_ptr<Impl> pImpl;
    };

    # widget.cpp
    #include"widget.h"
    #include"gadget.h"
    #include<string>
    #include<vector>

    struct Widget::Impl{
        std::string name;
        std::vector<double> data;
        Gadget g1, g2, g3;
    };

    Widget::Widget(): pImpl(std::make_unique<Impl>()) {}
    ```

- **std::unique\_ptr** 支持不完全类型
- **Pimpl Idiom** 是 **std::unqiue\_ptr** 常用场景之一
- 但是，简单的客户端程序引用 **Widget** 就会出错

    ```c++
    #include "widget.h"
    Widget w; // error!!!!!!
    ```

- 原因是：上面改写为智能指针的代码中，没有对 **Widget** 进行析构，因此编译器会自动生成析构函数，而在析构函数中，编译器会插入调用 **std::unqiue\_ptr** 的析构函数代码，默认的析构器是 **delete** ，然而通常默认 **delete** 会使用 **static\_assert** 来判断原始指针是否指向的是一个不完全类型，如果是就会报错，而且通常看到的错误是在构造 **Widget** 对象那一行，因为源码是显式的创建一个对象而隐式的销毁了该对象。为了解决这个问题，我们需要在析构函数调用时，确保 **Widget::pImpl** 是一个完整的类型，也就是当 **Widget** 的 **Impl** 在 **Widget.cpp** 中定义之后，类型是完整的，关键就是让编译器在看到 **Widget** 的析构函数之前先看到 **Widget::Impl** 的定义

    ```c++
    # widget.h
    class Widget {
        public:
            Widget();
            ~Widget();
            ...
        private:
            struct Impl;
            std::unique_ptr<Impl> pImpl;
    };

    # widget.cpp
    #include"widget.h"
    #include"gadget.h"
    #include<string>
    #include<vector>

    struct Widget::Impl{
            std::string name;
            std::vector<double> data;
            Gadget g1, g2, g3;
    };

    Widget::Widget(): pImpl(std::make_unique<Impl>()) {}
    Widget::~Widget() { }
    ```

- 上面的析构函数等价于默认析构函数，因此可以在实现中使用 **default** 来代替手动实现
- 但是，自定义析构函数后，就会使得编译器禁用自动生成移动构造函数，此时需要手动实现，但是不能在声明处使用 **default** ，因为和上面自动析构函数一样的问题，因此，在实现文件中使用 **default** 是可以的
- 如果要实现拷贝功能，则需要手动实现，因为编译器自动生成的拷贝函数不会拷贝那些只能移动的对象( **std::unique\_ptr** )
- 如果要将 **std::unique\_ptr** 替换成 **std::shared\_ptr** ，那么就不必做上面那么多工作了
    - **std::unique\_ptr** 中，自定义析构器是指针对象的一部分，要求在编译生成的特定函数中(析构函数，移动函数)指针指向的类型必须是完整的
    - **std::shared\_ptr** 中，自定义析构器不是指针对象的一部分，也就不要求在编译生成的特定函数(析构函数，移动函数)对象中指针指向的类型是完整的

## 7.Summary

- **std::unique\_ptr is a small, fast, move-only smart pointer for managing resources with exclusive-ownership semantics**
- **By default, resource destruction takes place via delete, but custom deleters can be specified. Stateful deleters and function pointers as deleters increase the size of std::unique\_ptr objects**
- **Converting a std::unique\_ptr to std::shared\_ptr is easy**
- **std::shared\_ptrs offer convenience approaching that of garbage collection for the shared lifetime management of arbitrary resources**
- **Compared to std::unique\_ptr, std::shared\_ptr objects are typically twice as big, incur overhead for control blocks, and require atomic reference count manipulations**
- **Default resource destruction is via delete, but custom deleters are supported. The type of the deleter has no effect on the type of the std::shared\_ptr**
- **Avoid creating std::shared\_ptrs from variables of raw pointer type**
- **Use std::weak\_ptr for std::shared\_ptr-like pointers that can dangle**
- **Potential use cases for std::weak\_ptr include caching, observer lists, and the prevention of std::shared\_ptr cycles**
- **Compared to direct use of new, make functions eliminate source code duplication, improve exception safety, and , for std::make\_shared and std::allocate\_shared, generate code that's smaller and faster**
- **Situations where use of make functions is inappropriate include the need to specify custom deleters and a desire to pass braced initializers**
- **For std::shared\_ptrs, additional situations where make functions may be ill-advised include (1) classes with custom memory management and (2) systems with memory concerns, very large objects, and std::weak\_ptrs that outlive the corresponding std::shared\_ptrs**
- **The Pimpl Idiom decreases build times by reducing compilation dependencies between class clients and class implementations**
- **For std::unique\_ptr pImpl pointers, declare special member functions in the class header, but implement them in the implementation file. Do this even if the default function implementations are acceptable.**
- **The above advice applies to std::unique\_ptr, but not to std::shared\_ptr**

## 8.Questions and Answers
1. 为什么要使用智能指针？
- 因为原始指针不能暗示指向的对象是单个对象还是一个数组，也不能暗示在什么时间点以什么样的方式来释放该对象，更不能保证不会重复释放该对象。因此，需要一种能够替我们自动管理内存的智能类，就是智能指针。

2. 智能指针的存储方式是什么样的？
- 对于**std::unique_ptr**来说，默认情况下只会保存原始指针，所以内存占用和原始指针一样大，仅当在构造时传入了自定义析构函数指针时，**std::unique_ptr**会额外保存自定义析构函数指针，所占内存是原始指针的两倍。而当传入了自定义析构器是函数对象时，所占内存大小取决于函数对象内存存储多少状态。
- 对于**std::shared_ptr**来说，直接占用的内存是原始指针大小的两倍，因为需要保存一个指向控制块的指针，该控制块内保存有共享引用计数，弱引用计数，自定义析构器，自定义分配器和虚函数等等。
- 对于**std::weak_ptr**来说，由于它和**std::shared_ptr**指向同一个对象，使用相同的控制块，因此，所占内存和**std::shared_ptr**一样，直接占用的内存是原始指针大小的两倍。

3. 论述**std::unique_ptr**、**std::shared_ptr**和**std::weak_ptr**三者之间的关系？
- 首先，**std::unique_ptr**用于独占式保存和访问指针，而**std::shared_ptr**用于保存和访问共享的资源，使用场景不一样，其次使用**std::weak_ptr**是为了解决**std::shared_ptr**的循环引用导致的内存泄露问题和**std::shared_ptr**无法判断对象是否已经释放的问题，**std::weak_ptr**通过检查引用计数是否为0来判断对象是否已经被析构。通常在判断对象释放和使用之间存在同步问题，因为检查判断结果和使用是两个分离的动作，因此实际使用时可能对象已经被释放了，因此，为了解决该问题，**std::weak_ptr**通过**lock**函数来生成一个新的**shared_ptr**来暂时锁定共享对象不被销毁，保证可以正常访问。

4.怎么解决**std::shared_ptr**潜在的重复绑定导致的多次释放相同资源问题？
- 尽量避免使用原始指针来构造智能指针，使用**std::make_shared**函数来构造，如果要使用原始指针来构造的话，一般在申请资源后立即将原始指针传递给智能指针，然后通过该智能指针来生成其他的智能指针，保证同一个资源只被共享在一个智能指针的控制块中。如果共享的指针是**this**指针，可以让类继承自标准库中的**std::enabled_shared_from_this**类，在使用**std::shared_ptr**的地方使用**shared_from_this()** 函数来代替**this**，并且将构造函数声明为**private**，然后通过调用工厂函数来创建对象并返回**std::shared_ptr**，因为**shared_from_this**要求必须提前存在一个**std::shared_ptr**对象的控制块，否则就是未定义行为。
