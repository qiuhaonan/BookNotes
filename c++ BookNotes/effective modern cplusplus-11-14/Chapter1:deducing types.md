## 1. Understand template type deduction.

- 函数模板的原型
    ```c++
    template<typename T>
    void f(ParamType param);
    ```
    - **ParamType**是一个左值引用或者指针时
        ```c++
        template<typename T>
        void f(T& param);

        int x = 27;
        const int cx = x;
        const int& rx = x;

        f(x);// T是int，param类型是int&
        f(cx);// T是const int, param类型是const int& 
        f(rx); // T是const int, param类型是const int&

        template<typename T>
        void f(const T& param);

        f(x); // T是int, param的类型是const int&
        f(cx); // T是int, param的类型是const int&
        f(rx); // T是int, param的类型是const int&

        const int* px = &x;
        f(&x); //T是int, param的类型是int*
        f(px); //T是const int, param的类型是const int*
        ```
        - 规则：
            - 如果函数调用的表达式中，实参是一个引用，那么就忽略掉引用部分
            - 然后把表达式和**ParamType**进行匹配来确定类型**T**.
    - **ParamType**是一个通用引用时
        ```c++
        template<typename T>
        void f(T&& param);

        int x = 27;
        const int cx = x;
        const int& rx = x;

        f(x);  //x是左值，T是int&，param类型是int&
        f(cx);//cx是左值，T是const int&，param类型是const int&
        f(rx); //rx是左值，T是const int&，param类型是const int&
        f(27);//27是右值，T是int，param类型是int&&
        ```
        - 此处的规则是按照通用引用参数的实例化规则来实现的
    - **ParamType**既不是指针也不是引用时
        ```c++
        template<typename T>
        void f(T param);

        int x = 27;
        const int cx = x;
        const int& rx = x;

        f(x);  // T和param的类型都是int
        f(cx);// T和param的类型都是int
        f(rx); // T和param的类型都是int
        ```
        - 规则：按值传递的时候，忽略掉参数的引用性，**const**属性，和**volatile**属性
        - 例外：
            ```c++
            template<typename T>
            void f(T param);

            const char* const ptr = "fun with pointers";

            f(ptr);
            //此时只会忽略掉指针本身的const
            //但是对于指向对象的const会被保留
            //即ptr的类型是const char*
            ```
    - 传入的参数是数组时
        ```c++
        const char name[] = "J.P.Briggs"; 
        //name的类型是const char[13]
        const char* ptrToName = name;  
        //数组蜕化成指针

        template<typename T>
        void f(T param);

        f(name); 
        //按值传递给模板函数的数组
        //模板参数会被推导成指针
        //name的类型是const char*

        template<typename T>
        void f(T& param);

        f(name); 
        //按引用传递给模板函数的数组
        //类型仍然是数组，name的类型是const char[13]
        //而模板参数类型是 const char(&)[13]
        ```
    - 传入的参数是函数时
        ```c++
        void someFunc(int, double);

        template<typename T>
        void f1(T param);

        template<typename T>
        void f2(T& param);

        // 参数被推导为函数指针，param类型是 void (*)(int, double)
        f1(someFunc); 
        // 参数被推导为函数引用，param类型是void (&)(int, double)
        f2(someFunc); 
        ```

## 2. Understand auto type deduction

- **auto**的推导方式几乎和模板函数推导方式一样，仅仅除了初始化列表的推导方式有所区别
    - 模板函数拒绝推导初始化列表的右值
    - **auto**可以将初始化列表推导为**std::initializer\_list<T>**
    - 例如：
        ```c++
        auto x1 = 27; //类型是int, 值是27
        auto x2 (27); //类型是int, 值是27
        auto x3 = {27}; //类型是std::initializer_list<int>，值是{27}
        auto x4 {27}; //类型是std::initializer_list<int>，值是{27}
        auto x5 = {1,2,3.0}; // 错误，值类型不止有一种
        auto x6 = {11,9,3}; //类型被推导为std::initializer_list<int>
        template<typename T>
        void f(T param);
        f({11,23,9}); //错误，模板函数拒绝推导
        template<typename T>
        void f(std::initializer_list<T> initlist);
        f({11,23,9}); //正确，参数被推导为std::initializer_list<int>
        ```
    - **C++14**允许函数的返回值使用**auto**来自动声明返回值类型，也允许对**lambda**表达式的参数使用**auto**自动声明，但是对于初始化列表类型仍然不能自动推导：
        ```c++
        auto createInitList()
        {
            return {1,2,3}; //错误，不能推导返回值类型为初始化列表的值   
        }

        std::vector<int> v;
        ...
        auto resetV = [&v](const auto& newValue) { v = newValue; };
        ...
        resetV({1,2,3}); //错误，不能推导模板函数为初始化列表的值
        ```

## 3. Understand decltype

- **C++**中**decltype**使用的第一个场景是声明一个函数模板，它的返回值类型依赖于参数类型，常见与**std::vector,** **std::deque**
    - 例子１：
        ```c++
        template<typename Container, typename Index>
        auto authAndAccess(Container& c, Index i) -> decltype(c[i]) 
        // 前面使用的auto和类型推导无关，它只是暗示语法使用了C++11的尾置返回类型
        {
            authenticateUser();
            return c[i];
        }

        //C++14的格式
        template<typename Container, typename Index>
        auto authAndAccess(Container& c, Index i)
        {
            authenticateUser();
            return c[i]; //本来应该返回对c[i]的引用
        }
        ```
    - 上述例子中 **return ci**的类型在推导时会忽略掉引用，结果返回的是一个右值，改进一下
        ```c++
        //C++14的做法
        template<typename Container, typename Index)
        decltype(auto) authAndAccess(Container& c, Index i)
        {
            authenticateUser();
            return c[i];　//可以返回引用，因为decltype会保留原始类型
        }
        ```
    - 但是如果想要既能返回右值，又能返回引用，仍然需要再修改一下
        ```c++
        //C++14做法
        template<typename Container, typename Index>
        decltype(auto) authAndAccess(Container&& c, Index i)
        {
            authenticateUser();
            return std::forward<Container>(c)[i]; 
            //因为不知道需要返回的是什么值，因此需要使用完美转发
            //当传入的参数是左值时，就返回引用，传入参数是右值时，就返回右值
        }

        //C++11
        template<typename Container, typename Index>
        auto authAndAccess(Container&& c, Index i) -> decltype
        (std::forward<Container>(c)[i])
        {
            authenticateUser();
            return std::forward<Container>(c)[i];
        }
        ```
- **decltype**也可以用于对变量类型的声明
    ```c++
    Widget w;
    const Widget& cw = w;
    auto myWidget1 = cw; //类型为Widget
    decltype(auto) myWidget2 = cw; //类型为const Widget&
    ```

- **decltype**失效的地方：
    - 对于返回值被**()**包围起来的值，会产生错误的自动推导类型，因为如果返回值本身是一个左值时，而**C++**定义表达式**(x)**也是一个左值，因此**decltype(x)**就是**int&**.
        ```c++
        decltype(auto) f1()
        {
            int x = 0;
            ...
            return x; //返回类型为int
        }

        decltype(auto) f1()
        {
            int x = 0;
            ...
            return (x); //返回类型为int&
        }　　
        ```

## 4. Know how to view deduced types

- 依靠编译器诊断
    - 利用模板类声明的不完全性，触发编译器警告
        ```c++
        template<typename T>
        class TD;

        TD<decltype(x)> xType; //编译器会报告x的类型
        TD<decltype(y)> yType;
        ```
    - 运行时输出，利用**typeid()**和**std::type\_info::name**来提供类型
        ```c++
        std::cout<< typeid(x).name();
        std::cout<< typeid(x).name();
        ```
