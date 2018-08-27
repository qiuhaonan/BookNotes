## 1.The vocabulary associated with lambdas

- **lambda** **expression**
    - 仅仅是一个表达式，是源码中一部分。
- **closure**
    - 是由一个**lambda**产生的运行时对象。
- **closure** **class**
    - 是一个类类型，一个**closure**可以从该**closure** **class**中实例化。每个**lambda**都会使得编译器产生一个独一无二的**closure** **class**。一个**lambda**内的语句会变成它的**closure** **class**的成员函数中可执行的指令。

## 2. Avoid default capture modes

- 默认的按引用传递能导致悬空引用
    - **lambda**表达式的生命周期大于引用的变量时，会出现悬空引用
        ```c++
        void addDivisorFilter()
        {
            auto calc1 = computeSomeValue1();
            auto calc2 = computeSomeValue2();
            auto divisor = computeDivisor(calc1, calc2);
            filters.emplace_back(
                [&](int value) { return value%divisor == 0;}
                ); // 悬空引用!!!
            filters.emplace_back(
                [&divisor](int value) {return value%divisor == 0;}
                ); //悬空引用!!!
        }
        ```
    - **lambda**表达式的生命周期跟引用的变量相同，但是**lambda**事后被拷贝用于其他地方时，会出现悬空引用
    - 正确做法是传值，但是要确保该值的生命周期不受外界的影响
- 默认的按值传递也会导致悬空指针
    - 传入的参数为指针时，当指针指向的对象的生命周期大于**lambda**表达式的生命周期时，会出现悬空指针
- 捕捉范围只能是非**static**局部变量
    - 隐式捕捉成员变量，虽然成员变量不是局部变量，编译也能通过，因为实际捕捉到的是指针，但是仍然有出错的可能

        ```c++
        class Widget {
            public:
                ...
                void addFilter() const;
            private:
                int divisor;
        };

        void Widget::addFilter() const
        {
            filters.emplace_back(
                [=](int value){ return value%divisor == 0; }
                );
            //编译器会将上面这行代码转换成
            auto currentObjectPtr = this;
            filters.emplace_back(
                [currentObjectPtr](int value) {
                    return value%currentObjectPtr->divisor == 0; 
                    }
                    );
        }

        using FilterContainer = 
        std::vector<std::function<bool(int)>>;
        FilterContainer filters;

        void doSomeWork()
        {
            auto pw = std::make_unique<Widget>();
            pw->addFilter(); //使用隐式捕捉成员变量
            ...// 出错，pw被销毁，lambda表达式现在持有的是悬空指针
        }
        ```
    - 显式捕捉或者默认捕捉成员变量会出错

        ```c++
        void Widget::addFilter() const
        {
        filters.emplace_back( 
            [divisor](int value) { return value%divisor == 0; }
            ); //错误，divisor不是局部变量
        }

        void Widget::addFilter() const
        {
        filters.emplace_back( 
            [ ](int value) { return value%divisor == 0; }
            ); //错误，默认捕捉无法捕捉非局部变量
        } 
        ```
    - 正确的捕捉成员变量方式是

        ```c++
        void Widget::addFilter() const
        {
            auto divisorCopy = divisor;
            filters.emplace_back( 
                [divisorCopy](int value) { 
                    return value%divisorCopy == 0; 
                    }
                );
        }

        void Widget::addFilter() const
        {
            auto divisorCopy = divisor;
            filters.emplace_back( 
                [=](int value) { 
                    return value%divisorCopy == 0; 
                    }
                );
        }
        ```
    - **C++14**中**lambda**可以带有内部成员变量
        ```c++
        void Widget::addFilter() const
        {
            filters.emplace_back(
                [divisor = divisor](int value) { 
                    return value%divisor == 0; 
                    }); 
                    //把Widget的divisor成员变量拷贝
                    //到lambda内部的成员变量divisor中
        }
        ```
    - **lambda**也不能捕捉具有静态存储周期的对象，比如全局对象，命名空间范围的对象，或者被声明为**static**属性的对象(无论是在类内部，函数内部还是文件内部)，但是能不能使用要看具体情况

        ```c++
        void addDivisorFilter()
        {
            static auto calc1 = computeSomeValue1();
            static auto calc2 = computeSomeValue2();

            static auto divisor = computeDivisor(calc1, calc2); 
            filters.emplace_back(
                [=](int value) { 
                    return value%divisor == 0; 
                    }); 
                    //捕捉不到任何对象，但是可以在lambda内部
                    //使用这个静态对象，而且是按照引用而不是值来使用的
            ++divisor;
        }
        ```

## 2. Use init capture to move objects into closures

- 如果要传递一个只能移动的对象，那么按值和引用传递都不能满足**lambda**的捕捉方式
    - **C++14**的初始化捕捉

        ```c++
        class Widget {
            public:
            ...
            bool isValidated() const;
            bool isProcessed() const;
            bool isArchived() const;
            
            private:
            ...
        };

        auto pw = std::make_unique<Widget>();
        ...
        auto func = [pw = std::move(pw)] {
            return pw->isValidated() && pw->isArchived(); 
            }; 
            //在lambda类内部生成一个pw成员变量
            //然后接管外部变量pw的右值

        //or
        auto func = [pw = std::make_unique<Widget>()] { 
            return pw->isValidated() && pw->isArchived(); 
            }; 
            //直接使用表达式返回的右值对lambda内部成员变量进行初始化
        ```
        - 规则：
            - 指定从**lambda**产生的闭包类的数据成员名字
            - 使用一个表达式对这个数据成员进行初始化
    - **C++11**的**lambda**表达式不能捕捉一个表达式的返回值或者一个只能移动的对象，但是一个**lambda**表达式只是一种简单的方式来生成一个类和这个类的对象，因此有其他的替代方法
        - 替代方法：

            ```c++
            class IsValAndArch {
                public:
                    using DataType = std::unique_ptr<Widget>;
                    explicit IsValAndArch(DataType&& ptr):
                    pw(std::move(ptr)) {}
                    bool operator()() const
                    {
                        return pw->isValidated() && pw->isArchived();
                    }
                private:
                    DataType pw;
            };

            auto func = IsValAndArch(std::make_unique<Widget>());
            ```
        - 如果仍然要使用**lambda**表达式，又想捕捉到移动对象，需要借助另一个工具**std::bind**

            ```c++
            std::vector<double> data;
            ...
            auto func = std::bind(
                [](const std::vector<double>& data) {...}, 
                std::move(data));
            ```
            - 方法规则：
                - 把要捕捉的对象移动到由**std::bind**产生的一个函数对象中
                - 把这个捕捉对象的引用传递给给**lambda**表达式
            - 解释：
                - 一个绑定对象包含传递给**std::bind**的所有参数的拷贝
                - 对于每一个左值参数，在**bind**里面的对应对象是拷贝构造的
                - 对于每一个右值参数，在**bind**里面的对应对象是移动构造的
                - 当一个**bind**对象被调用的时候，**bind**内部存储的参数就被传递给这个调用对象(**bind**绑定的)
                - 传递给**lambda**的参数是左值引用，因为虽然传递给**bind**的参数是右值，但是对应的内部参数本身是一个左值。
                - 默认情况下，从**lambda**表达式产生的闭包类的内部成员函数**operator()** ，是**const**属性的，这使得闭包里面的所有数据成员在**lambda**体内都是**const**属性的，而**bind**对象里面移动过来的**data**不是**const**的，为了防止在**lambda**内部对**data**进行修改，需要加上**const**
                - 如果**lambda**被声明为**mutable**，闭包类里面的**operator()** 就不会被声明为**const**，那么也就不必对**lambda**的参数加上**const**声明

                    ```c++
                    auto func = std::bind(
                        [] (std::vector<double>& data) mutable {...}, 
                        std::move(data));　　　
                    ```
                - bind对象的生命周期和闭包的声明周期一致

## 3. Use decltype on auto&& parameters to std::forward them

- **C++14**支持泛型**lambda**表达式
- 对**lambda**表达式使用**auto**来声明参数
    - 实现例子：

        ```c++
        auto f = [](auto x) {return normalize(x); };

        //编译器编译后是这样
        class SomeCompilerGeneratedClassName {
            public:
                template<typename T>
                auto operator()(T x) const
                {
                    return normallize(x);
                }
            ...
        };
        ```
    - 操作符 **()**  在 **lambda** 的闭包类中是一个模板，但是如果**normalize**函数区分左值参数和右值参数，上面的写法不完全对，要实现完美转发的话需要做两点改动
        - 把**x**声明为一个通用引用
        - 使用**std::forward**把**x**转发给**normalize**函数，结果如下：

            ```c++
            auto f = [](auto&& x) { 
                return normalize(std::forward<???>(x)); 
                }; 
                // ???应该填入x的类型，但是这个类型不是固定的
                //且此处也不是模板函数
            ```
    - 通过**decltype**来确定参数的类型名和左值/右值属性
        - 过程：

            ```c++
            auto f = [](auto&& x) { 
                return normalize(std::forward<decltype(x)>(x); 
                };
                
            //1,decltype推导x的类型A
            //2.std::forward根据A推导模板参数类型T
            ```
        - **decltype**作用在左值参数，得到左值引用类型；作用在右值参数，得到右值引用类型
        - **std::forward**函数中**T**应该使用左值引用来暗示参数是左值，**T**应该使用非引用来暗示参数是右值
        - 左值作用在通用引用，得到左值引用参数；右值作用在通用引用参数，得到右值引用参数
        - 尽管**decltype**在把右值参数推导为右值引用类型而不是非引用类型 **(std::forward\<T>** 中 **T** 要求的)，但是最终转发的结果一样
    - 如果要转发可变参数列表时，使用...来代替

        ```c++
        auto f = [](auto&&... xs) { 
            return normalize(std::forward<decltype(xs)>(xs)...); 
            };
        ```

## 4. Prefer lambdas to std::bind

- 现在有一个闹钟程序如下：

    ```c++
    using Time = std::chrono::steady_clock::time_point;
    enum class Sound {Beep, Siren, Whistle};
    using Duration = std::chrono::steady_clock::duration;
    void setAlarm(Time t, Sound s, Duration d); 
    //设置一个闹钟，在时间t以铃声s开始响，最长持续时间为d
    ```
    - 如果需要一个新的函数在上述基础之上来实现延迟一个小时再开始响，持续时间改为30秒
        - 使用**lambda**表达式实现

            ```c++
            auto setSoundL = [](Sound s){
                using namespace std::chrono;
                setAlarm(steady_clock::now() + hours(1), s, seconds(30));
            };
            ```
        - 使用**std::bind**来实现

            ```c++
            using namespace std::chrono;
            using namespace std::literals;
            using namespace std::placeholders;

            auto setSoundB = std::bind(setAlarm, 
                steady_clock::now() + 1h, _1, 30s); 
            ```
            - 按照上面的写法，闹钟将会在从**bind**函数时刻推迟**1**个小时开始响，而不是**setAlarm**函数调用时刻开始算起向后推迟**1**个小时，因为**bind**会把传入的参数拷贝到**bind**对象内部，以后调用的时候再把这些参数传递给可调用对象
            - 一种修正方法是让**bind**延迟解析表达式的值，直到**setAlarm**被调用的时候再解析，**C++14**的写法

                ```c++
                auto setSoundB = std::bind(setAlarm, 
                    std::bind(std::plus<>(), 
                        std::bind(steady_clock::now), 
                            1h), _1, 30s);
                ```
            - 上面将**steady\_clock::now**作为可调用对象传给**bind**，而不是作为参数表达式传入，这样可以在调用外部**setAlarm**对象时，即时生成内部**bind**的结果，从而达到延迟解析效果
            - **C++11**的写法

                ```c++
                using namespace std::chrono;
                using namespace std::placeholders;

                auto setSoundB = std::bind(
                    setAlarm, 
                    std::bind(
                        std::plus<steady_clock::time_point>(),             
                        std::bind(steady_clock::now), hours(1)
                        ), 
                    _1, 
                    seconds(30));
                ```
    - 假设**setAlarm**有重载函数，接收**4**个参数的话: **void setAlarm(Time t, Sound s, Duration d, Volume v);** 其中**enum class Volume { Normal, Loud, LoudPlusPlus };**
        - **lambda**表达式写法

            ```c++
            auto setSoundL = [](Sound s){
                using namespace std::chrono;
                setAlarm(steady_clock::now() + 1h, s, 30s); 
                //能够正确匹配
            };
            ```
        - **bind**如果仍然按照上面的写法会出错，因为编译器只知道函数名，对于传入的参数个数不能根据传递给**bind**的参数个数确定，修正做法是对调用的函数名转换成函数指针，做强制类型指定

            ```c++
            using SetAlarm3ParamType = void(*) (Time t, Sound s, Duration d);
            auto setSoundB = std::bind(
                static_cast<SetAlarm3ParamType>(setAlarm), 
                std::bind(
                    std::plus<>, 
                    std::bind(steady_clock::now), 
                    1h),
                _1, 
                30s);
            ```
        - 但是，编译器更有可能对函数名做**inline**函数调用，不太可能对函数指针做这种优化，因此使用**lambda**的代码在这种情况下要比**bind**快
    - **C++11**中，**bind**的用途主要在于实现移动捕捉或把模板函数调用绑定到对象上

## 5. Summary

- **Default by-reference capture can lead to dangling references.**
- **Default by-value capture is susceptible to dangling pointers (especially this), and it misleadingly suggests that lambdas are self-contained.**
- **Using C++14's init capture to move objects into closures**
- **In C++11, emulate init capture via hand-write classes or std::bind**
- **Use decltype on auto&& parameters to std::forward them**
- **Lambdas are more readable, more expressive, and may be more efficient than using std::bind**
- **In C++11 only, std::bind may be useful for implementing move capture or for binding objects with templatized function call operators.**
