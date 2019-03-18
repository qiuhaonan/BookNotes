## 1. Distinguish between () and {} when creating objects

- **C++11**中，初始化值的指定方式有三种：括号初始化，等号初始化和花括号初始化；其中花括号初始化是为了解决**C++98**的表达能力而引入的一种统一初始化思想的实例。
    - 等号初始化和花括号初始化可以用于非静态成员变量的初始化
        ```c++
        class Widget {
        ...
        private:
            int x {0}; // ok
            int y = 0; // ok
            int z(0); // error
        };
        ```
    - 括号初始化和花括号初始化可以用于不可拷贝对象的初始化 
        ```c++
        std::atomic<int> ai1 {0}; // ok
        std::atomic<int> ai2 (0); //ok
        std::atomic<int> ai3 = 0; // error
        ```
    - 花括号初始化会禁止窄化转型，而等号初始化和括号初始化会自动窄化转型
        ```c++
        double x, y, z;
        ...
        int sum1 {x+y+z}; // error
        int sum2 (x+y+z); // ok
        int sum3 = x+y+z; // ok
        ```
    - 调用对象的无参构造函数时，使用括号初始化会被编译器错误识别为声明了一个函数，而花括号初始化则能正确匹配到无参构造函数的调用
        ```c++
        Widget w1(); // error
        Widget w2{}; // ok 
        ```
    - 花括号初始化与**std::initializer\_lists**和构造函数重载解析的同时出现时容易造成错误调用
        - 在调用构造函数的时候，只要不涉及到**std::initializer\_list**参数，括号和花括号初始化有相同的含义
            ```c++
            class Widget {
                public:
                    Widget(int i, bool b);
                    Widget(int i, double d);
                    ...
            };

            Widget w1(10, true); // calling 1
            Widget w2{10, true}; // calling 2
            Widget w3(10, 5.0);  // calling 1
            Widget w4{10, 5.0};  // calling 2
            ```
        - 如果涉及到**std::initializer\_list**参数，在使用花括号初始化时，编译器会强烈地偏向于调用使用**std::initializer\_list**参数的重载构造函数
            ```c++
            class Widget {
                public:
                    Widget(int i, bool b);
                    Widget(int i, double d);
                    Widget(std::initializer_list<long double> il);
                    ...
            };

            Widget w1(10, true); // calling 1
            Widget w2{10, true}; // calling 3, 10 and true convert to long double
            Widget w3(10, 5.0);  // calling 1
            Widget w4{10, 5.0};  // calling 3 , 10 and 5.0 convert to long double
            ```
            - 甚至本来应该调用拷贝构造函数或者移动构造函数，也会被**std::initializer\_list**构造函数给劫持
                ```c++
                Widget w5(w4); // copy construction 
                Widget w6{w4}; // std::initializer_list construction
                Widget w7(std::move(w4)); // move construction
                Widget w8{std::move(w4)}; // std::initializer_list construction
                ```
            - 编译器非常偏向选择**std::initializer\_list**构造函数，以至于即便最匹配的**std::initializer\_list**构造函数不能被调用，编译器也会优先选择它
                ```c++
                class Widget {
                public:
                    Widget(int i, bool b);
                    Widget(int i, double d);
                    Widget(std::initializer_list<bool> il);
                    ...
                };

                Widget w{10, 5.0}; // error, requires narrowing conversions
                ```
            - 只有当没有办法在花括号初始化的参数类型和**std::initializer\_list**的参数类型之间进行转换时，编译器才会重新选择正常的构造函数
                ```c++
                class Widget {
                public:
                    Widget(int i, bool b);
                    Widget(int i, double d);
                    Widget(std::initializer_list<std::string> il);
                    ...
                };

                Widget w1(10, true); // calling 1
                Widget w2{10, true}; // calling 1
                Widget w3(10, 5.0);  // calling 2
                Widget w4{10, 5.0};  // calling 2
                ```
            - 当类同时支持默认构造函数和**std::initializer\_list**构造函数时，此时调用空的花括号初始化，编译器会解析为调用默认构造函数，而要解析成**std::initializer\_list**构造函数，需要在花括号中嵌套一个空的花括号进行初始化
                ```c++
                class Widget {
                    public:
                        Widget();
                        Widget(std::initializer_list<int> il);
                        ...
                };

                Widget w1; // calling 1
                Widget w2{}; // calling 1
                Widget w3{{}}; // calling 2
                Widget w4({}); // calling 2
                ```

## 2. Prefer nullptr to 0 and NULL

- **C++**会在需要指针的地方把**0**解释成指针，但是需要策略还是把**0**解释成**int**型
- **C++98**中上面这种做法会使得在指针和**int**型重载共存时产生意外匹配调用
    ```c++
    void f(int);
    void f(bool);
    void f(void*);
    f(0);  // calls f(int)
    f(NULL); // might not compile, but typically calls f(int)
    ```
- **nullptr**的优点在于它没有一个整型类型，也没有一个指针类型，但是可以代表所有类型的指针，**nullptr**的实际类型是**nullptr\_t**，可以被隐式地转换成所有原始指针类型
    ```c++
    f(nullptr); // calls f(void*)
    ```
- 当在使用模板时，**nullptr**的优势就发挥出来了，可以转换成任意指针类型
    ```c++
    int f1(std::shared_ptr<Widget> spw);
    int f2(std::unique_ptr<Widget> upw);
    bool f3(Widget* pw);
    std::mutex f1m, f2m, f3m;

    template<typename FuncType, typename MuxType, typename PtrType>
    auto lockAndCall(FuncType func, MuxType& mutex, PtrType ptr) -> decltype(func(ptr))
    {
        using MuxGuard = std::lock_guard<MuxType>;
        MuxGuard g(mutex);
        return func(ptr);
    }

    auto result1 = lockAndCall(f1, f1m, 0); // error, PtrType is int
    auto result2 = lockAndCall(f2, f2m, NULL); // error, PtrType is int / long
    auto result3 = lockAndCall(f3, f3m, nullptr); // ok 
    ```

## 3. Prefer alias declarations to typedefs

- **alias**比**typedef**更容易理解
    ```c++
    typedef void (*FP)(int, const std::string&);
    using FP = void(*)(int, const std::string&);
    ```
- **alias**可以模板化，而**typedef**不能直接模板化，需要借助结构体来实现
    - 如果要定义一个使用自定义分配器的链表
        ```c++
        template<typename T>
        using MyAllocList = std::list<T, MyAlloc<T>>;
        MyAllocList<Widget> lw;

        template<typename T>
        struct MyAllocList {
        typedef std::list<T, MyAlloc<T>> type;
        };
        MyAllocList<Widget>::type lw;
        ```
    - 如果要在模板内部创建一个持有模板参数类型的链表，必须在**typedef**名字前面加上**typename**
        ```c++
        template<typename T>
        class Widget {
            private:
                typename MyAllocList<T>::type list;
                ...
        };
        ```
        - **MyAllocList<T>::type**指的是一个取决于模板类型参数**T**的类型，因此就是一个依赖类型，**C++**规定依赖类型前面必须加上**typename**
        - 如果使用**alias**定义模板，就不需要**typename**了
            ```c++
            template<typename T>
            using MyAllocList = std::list<T, MyAlloc<T>>; 

            template<typename T>
            class Widget {
            private:
                MyAllocList<T> list;
                ...
            };
            ```
        - 此处看起来**MyAllocList<T>**是一个与模板参数**T**存在依赖关系的对象，但是当编译器处理**Widget**模板时，它知道**MyAllocList<T>**是一个类型的名字，因为**MyAllocList**是一个别名模板**:**它必须命名一个类型，因此**MyAllocList<T>**是一个无依赖类型，也就不需要**typename**了
        - 在**typedef**中，当编译器在**Widget**模板中看到**MyAllocList<T>::type**时，它们不能确定这是否是一个类型，因为有可能是**MyAllocList<T>**的一个特例而它们没看到，例如：
            ```c++
            class Wine{...};

            template<>
            class MyAllocList<Wine> {
                private:
                    enum class WineType {White, Red, Rose};
                    WineType type;  //!!!!!!!!!!!!!!!
                    ...
            };
            ```
- **C++11**以类型萃取的形式提供了许多形式转换工具，模板都在**<type\_traits>**头文件中，例如
    ```c++
    std::remove_const<T>::type
    std::remove_reference<T>::type
    std::add_lvalue_reference<T>::type
    ```
    - 但是要在模板内部使用它们时，仍然要在前面加上**typename**，因为它们实际上还是用嵌套**typedef**实现的
    - 但在**C++14**中，它们有了替代的方案
        ```c++
        std::remove_const_t<T>
        std::remove_reference_t<T>
        std::add_lvalue_reference_t<T>
        ```
        - 原理显而易见
            ```c++
            template<class T>
            using remove_const_t = typename remove_const<T>::type;

            template<class T>
            using remove_reference_t = typename remove_reference<T>::type;

            template<class T>
            using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;
            ```

## 4. Prefer scoped enums to unscoped enums

- 通常情况下，在花括号内声明一个名字可以限制名字对外的可见性，但是对于**C++98**的**enums**中的**enumerators**并非如此，其对外也是可见的
    ```c++
    enum Color {black, white, red};
    auto while = false; // error, while already declared in this scope
    ```
- **C++11**的新标准，有范围限制的**enums**，并不会对命名空间造成污染
    ```c++
    enum class Color {black, white, red};
    auto white = false; // fine
    Color c = white; // error, no enumerator named “white" is in this scope
    Color c = Color::white; // fine
    auto c = Color::white; // fine
    ```
- 有范围限制**enums**中的枚举常量有更强的类型，而对于无范围限制的**enums**中枚举常量会被隐式转换成整型类型
    ```c++
    enum Color {black, white, red};
    std::vector<std::size_t> primeFactors(std::size_t x);

    Color c = red;
    ...
    if( c < 14.5){ // compare Color to double!!
    auto factors = primeFactors(c); // compute prime factors of a Color!!
    ...
    }

    enum class Color {black, white, red};
    Color c = Color::red;
    ...
    //error, can't compare Color and double!!!
    if( c < 14.5){ 
    //error, can't pass Color to function expecting std::size_t
        auto factors = primeFactors(c); 
        ...
    }
    ```
- 如果要把**C++11**中的**enums**变量转换成其他类型，需要使用**static\_cast<>()**
    ```c++
    if( static_cast<double>(c) < 14.5 ){ // valid
        auto factors = primeFactors(static_cast<std::size_t>(c)); // valid
        ...
    }
    ```
- **C++**中每个**enum**都有一个由编译器决定的整型底层类型，为了有效利用内存，编译器通常会选择足够代表枚举量范围的最小的底层类型，为此，**C++98**只支持**enum**定义(列出所有的枚举值)，而不支持声明，这使得在使用**enum**前，编译器能选择一个底层类型。
- 无法对**enum**前置声明有许多缺点，最显著的就是增加编译的依赖性，如果一个**enum**被系统中每个组件都有可能用到，那么都得包含这个**enum**所在的头文件，如果需要新加入一个枚举值，整个系统就有可能重新编译，即便只有一个函数使用这个新的值
- **C++11**中的**enum**类可以消除这个编译需求，例如
    ```c++
    #file 1
    enum Status {
    good = 0,
    failed = 1,
    incomplete = 100,
    corrupt = 200,
    audited = 500,
    indeterminate = 0xFFFFFFFF
    };

    #file 2
    enum class Status;
    void continueProcessing(Status s);
    ```
    - 如果修改了**Status**，而且**continueProcessing**没有使用到新的值，那么**file2**就不需要重新编译
    - 但是如果编译器在使用一个**enum**之前，需要知道它的大小该怎么办？
        - 对于一个有范围限制的**enum,**它的底层类型是已知的(默认是**int**，可以手动覆盖)，而对于没有范围限制的**enum**，底层类型可以指定
            ```c++
            enum class Status; //int, declaration 
            enum class Status: std::uint32_t; //uin32_t, declaration
            enum Color: std::uint8_t;// uint8_t, declaration

            enum class Status: std::uint32_t {
                good = 0,
                failed = 1,
                incomplete = 100,
                corrupt = 200,
                audited = 500,
                indeterminate = 0xFFFFFFFF
            };
            ```
- 无范围限制的**enum**在**C++11**的**std::tuples**中的用途
    ```c++
    using UserInfo = std::tuple<std::string, std::string, std::size_t>; // name, email, reputation
    UserInfo uInfo;
    ...
    //get value of field 1
    //but can you always remember what the hell 1 represents?
    auto val = std::get<1>(uInfo); 

    //Improve
    enum UserInfoFields {uiName, uiEmail, uiReputation};
    UserInfo uInfo;
    ...
    //implicit conversion from UserInfoFields to std::size_t
    //which is the type that std::get requires
    auto val = std::get<uiEmail>(uInfo); 
    ```
    - 如果要改写成有范围限制的**enum**，略显拖沓
        ```c++
        enum class UserInfoFields {uiName, uiEmail, uiReputation};
        UserInfo uInfo;
        ...
        auto val = std::get<static_cast<std::size_t>(UserInfoFields::uiEmail)>(uInfo);
        ```

## 5. Prefer deleted functions to private undefined ones

- 删除的函数和声明为**private**的函数之间的区别
    - 删除的函数在任何地方都不能使用，所以成员函数和友元函数都不能使用已经删除的函数，否则会编译失败，这在**C++98**中会推迟到链接阶段才会报错
    - 删除的函数是**pulic**而不是**private**，因为当客户端代码试图使用这个删除的成员函数时，**C++**会首先检查访问权限，后检查删除状态，如果设为**private**，编译器给出的是权限不足警告而不是函数不可用警告
    - 任何函数都可以是**deleted**状态，而只有成员函数可以是**private**，例如删除某些过时的重载函数
        ```c++
        bool isLucky(int number);
        bool isLucky(char) = delete;
        bool isLucky(bool) = delete;
        bool isLucky(double) = delete;
        ```
    - 虽然删除的函数不能使用，但仍然是程序的一部分，因此，在重载解析过程中也会被纳入考虑中
    - 模板函数可以通过删除来阻止部分实例化函数，而允许其他实例化存在
        ```c++
        template<typename T>
        void processPointer(T* ptr);

        template<>
        void processPointer<void>(void*) = delete;

        template<>
        void processPointer<char>(char*) = delete;
        ```
    - 有意思的是，如果在类里面有一个模板函数，则不能通过设置**private**来禁用一些实例化，因为不能给一个成员函数的模板特化一个不同于主模板的访问权限，例如
        ```c++
        class Widget {
            public:
                ...
                template<typename T>
                void processPointer(T* ptr) {...}
            private:
                template<>
                void processPointer<void>(void*); // error
        };
        ```
        - 问题在于模板特化必须被卸载命名空间范围内，而不是在类范围内，因此可以使用**delete**来实现
            ```c++
            class Widget {
                public:
                    ...
                    template<typename T>
                    void processPointer(T* ptr) {...}
                    ...  
            };

            template<>
            void Widget::processPointer<void>(void*) = delete;
            ```

## 6. Declare overriding functions override

- 覆盖产生的必要条件
    - 基类函数必须是**virtual**的
    - 基类和派生类的函数名必须一致
    - 基类和派生类函数的参数类型必须一致
    - 基类和派生类函数的**const**属性必须一致
    - 基类和派生类函数的返回类型以及异常说明必须兼容
    - 函数的引用修饰必须一致**(C++11)**
        - 限制成员函数的使用只能是左值或者右值**(\*this)**
            ```c++
            class Widget {
            public:
            ...
            void doWork() &; // only when *this is an lvalue
            void doWork() &&; // only when *this is an rvalue
            };

            ...
            Widget makeWidget();
            Widget w;
            ...
            w.doWork();
            makeWidget().doWork();
            ```

- 显式地对成员函数声明**override**能使得编译器检查是否正确覆盖，而不是在没有正确覆盖时隐式地转换成了重载或者其他合法函数，而使得调用时发生意外调用，例如
    ```c++
    class Base{
    public:
        virtual void mf1() const;
        virtual void mf2(int x);
        virtual void mf3() &;
        void mf4() const;
    };

    class Derived: public Base {
    public:
        virtual void mf1();  // not const 
        virtual void mf2(unsigned int x);  // not int
        virtual void mf3() &&; // not &
        void mf4() const; // not virtual in base
    };
    ```
    - 虽然上面的函数都没有发生覆盖，但是有些编译器认为都是合法的，而不会给出警告，正确的做法是
        ```
        class Derived: public Base {
            public:
                virtual void mf1() override;
                virtual void mf2(unsigned int x) override;
                virtual void mf3() && override;
                virtual void mf4() const override;
        };
        ```
        - 此时，编译器能检查出所有的错误覆盖

## 7. Prefer const\_iterators to iterators

## 8. Declare functions noexcept if they won't emit exceptions

## 9. Use constexpr whenever possible

- 对于**constexpr**对象，它们具有**const**属性，并且它们的值在编译的时候确定(从技术角度讲，是在转换期间确定，转换期包括编译和链接)，它们的值也许会被放在只读内存区中，它们的值也能被用在整型常量表达式中，例如数组长度，整型模板参数，枚举值，对齐指示符等等
- 当**constexpr**函数使用**constexpr**对象时，它们会产生编译期常量，如果**constexpr**函数使用了运行时的值，它们就会产生运行时的值，但是如果**constexpr**函数使用的所有参数都是运行时的值，那么就会报错
- 在**C++11**中，**constexpr**函数只能包含不超过一条**return**语句的执行语句，但是可以使用条件运算符和递归来实现多重运算。
- 在**C++14**中，**constexpr**函数的语句数量没有限制，但是函数必须接收和返回字面值类型，也就是指可以在编译期间确定值的类型。
- 字面值类型包括除了**void**修饰的类型和带有**constexpr**修饰的用户自定义类型(因为构造函数和其他成员函数也可能是**constexpr**)
    ```c++
    class Point {
    public:
        constexpr Point(double xVal = 0, double yVal = 0) noexcept: x(xVal), y(yVal) {}
        constexpr double xValue() const noexcept { return x;}
        constexpr double yValue() const noexcept { return y;}
        void setX(double newX) noexcept { x = newX;}
        void setY(double newY) noexcept { y = newY;}
    private:
        double x, y;
    };
    constexpr Point p1(9.4, 2.7);
    constexpr Point p2(28.8, 5.3);

    constexpr Point midpoint(const Point& p1, const Point& p2) noexcept
    {
        return { (p1.xValue() + p2.xValue()) / 2, (p1.yValue() + p2.yValue()) / 2 };
    }

    constexpr auto mid = midpoint(p1, p2);
    ```

- **C++11**中，**setX**和**setY**不能被声明为**constexpr**，因为不能在**const**成员函数中修改成员变量，而且返回值为**void**，并不是字面值常量，但是**C++14**中是允许的

## 10. Make const member functions thread safe

## 11. Understand special member function generation

- 特殊成员函数是**C++**会自动生成的函数，**C++98**中有四个这样的函数：默认构造函数，析构函数，拷贝构造函数，拷贝赋值运算符；**C++11**中多了两个：移动构造函数和移动赋值运算符
- 两个拷贝操作是无关的，声明一个不会阻止编译器产生另一个
- 两个移动操作是相关的，声明一个会阻止编译器自动产生另一个
- 显式声明一个拷贝操作后，移动操作就不会被自动生成，反之依然，理由是：比如声明了拷贝运算，就说明移动操作不适合用于此类
- 三条规则：如果声明了拷贝构造，拷贝赋值或者析构函数中任何一个，都应该将三个一起声明，因为这三个函数是相互关联的
- 三条规则暗示了析构函数的出现使得简单的**memberwise**拷贝不适合类的拷贝操作，也就是说如果声明了析构函数，那么就不应该自动生成拷贝操作相关的函数，因为可能会存在不一致的资源管理行为。同样的，也不应该自动生成移动操作相关的函数。所以，只有当类满足下面三个条件时，移动操作才会自动生成：
    - 没有声明拷贝操作
    - 没有声明移动操作
    - 没有声明析构函数
- 假如编译器生成的函数行为正确，那么我们只需要在函数名后面加上**default**就可以了，然编译器接管一切具体事务。

## 12. Summary

- **Braced initialization is the most widely usable initialization syntax, it prevents narrowing conversions, and it's immune to C++'s most vexing parse.**
- **During constructor overload resolution, braced initializers are matched to std::initializer\_list parameters if at all possible, even if other constructors offer seemingly better matches.**
- **An example of where the choice between parentheses and braces can make a significant difference is creating a std::vector<numeric type> with two arguments.**
- **Choosing between parentheses and braces for object creation inside templates can be challenging.**
- **typedefs don't support templatization, but alias declarations do.**
- **Alias templates avoid the "::type" suffix and, in templates, the "typename" prefix often required to refer to typedefs.**
- **C++14 offers alias templates for all the C++11 type traits transformations.**
- **C++98-style enum are now known as unscoped enums.**
- **Enumerators of scoped enums are visible only within the enum. They convert to other types only with a cast.**
- **Both scoped and unscoped enums support specification of the underlying type. The default underlying type for scoped enums is int. Unscoped enums have no default underlying type.**
- **Scoped enums may alaways be forward-declared. Unscoped enums may be forward-declared only if their declaration specifies an underlying type.**
- **Prefer deleted functions to private undefined ones.**
- **Any function may be deleted, including non-member functions and template instantiations.**
- **Declare overriding functions override.**
- **Member function reference qualifiers make it possible to treat lvalue and rvalue objects (\*this) differently.**
- **Prefer const\_iterators to iterators.**
- **In maximally generic code, prefer non-member versions of begin, end, rbegin, etc., over their member function counterparts.**
- **noexcept is part of a function's interface, and that means that callers may depend on it.**
- **noexcept functions are more optimizable than non-noexcept functions.**
- **noexcept is particularly valuable for the move operations, swap, memory deallocation functions, and destructors.**
- **Most functions are exception-neutral rather than noexcept.**
- **constexpr objects are const and are initialized with values known during compilation.**
- **constexpr functions can produce compile-time results when called with arguments whose values are known during compilation.**
- **constexpr objects and functions may be used in a wider range of contexts than non-constexpr objects and functions.**
- **constexpr is part of an object's or function's interface.**
- **Make const member functions thread safe unless you're certain they'll never be used in a concurrent context.**
- **Use of std::atomic variables may offer better performance than a mutex, but they're suited for manipulation of only single variable or memory location.**
- **The special member functions are those compilers may generate on their own: default constructor, destructor, copy operations, and move operations.**
- **Move operations are generated only for classes lacking explicitly declared move operations, copy operations, or a destructor.**
- **The copy constructor is generated only for classes lacking an explicitly declared copy constructor, and it's deleted if a move operation is declared. The copy assignment operator is generated only for classes lacking an explicitly declared copy assignment operator, and it's deleted if a move operation is declared. Generation of the copy operations in classes with an explicitly declared destructor is deprecated.**
- **Member function templates never suppress generation of special member functions.**
