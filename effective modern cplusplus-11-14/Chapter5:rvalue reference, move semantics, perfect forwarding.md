## 1. 理解std::move和std::forward

- 从**std::move**和**std::forward**不能做的地方开始入手是有帮助的，**std::move**不会移动任何值，**std::forward**也不会转发任何东西，在运行时，他们不会产生可执行代码，一个字节也不会 **:)** 。他们实际上是执行转换的函数模板。**std::move** 无条件的把它的参数转换成一个右值，而**std::forward**在特定条件下将参数转换成右值。

    ```c++
    //c++11中std::move的简化版本
    template<typename T>
    typename remove_reference<T>::type&& move(T&& param) 
    //返回类型加上&&表明是一个右值引用
    {
        // 当T本身是一个左值引用时，T&&就是一个左值引用
        // 为了确保&&不会被应用到一个引用类型上
        // 需要使用remove_reference来做类型萃取
        using ReturnType = typename remove_reference<T>::type&&; 
        //保证返回值是一个右值引用，该值本身就是右值
        return static_cast<ReturnType>(param);
    }
    //c++14中std::move的简化版本
    template<typename T>
    decltype(auto) move(T&& param) //返回值类型推导
    {
    //标准库别名模板
    using ReturnType = remove_reference_t<T>&&; 
    return static_cast<ReturnType>(param);
    }
    ```

- **std::move**和**std::forward**不能用于**const**对象，因为把一个对象的值给移走，本质上就是对该对象进行修改，所以语言不允许对函数修改传递给他们的**const**对象，例如：

    ```c++
    class Annotation
    {
        public:
            //通过拷贝将参数按值传递并设为const防止修改
            explicit Annotation(const std::string text) 
            :value(std::move(text)) 
            // 使用移动操作避免重复拷贝，但是move不改变const属性
            //仅仅是将const std::string左值转变成const std::string
            //右值，即传递给value构造函数的仍然是const std::string
            {
                ...     // construction 
            }
        private:
            std::string value;
    };

    class string
    {
            public:
            ...
            // const左值引用可以绑定到const 右值
            string(const string& rhs); 
            // 不接受const类型的参数
            string(string&& rhs);  
    }; 
    ```
    - 实现结果：

        ```c++
        #include <iostream>

        using namespace std;

        class A
        {
            public:
                A(const string& a)
                {
                    cout<<"copy construct"<<endl;
                    cout<<"para:"<<a<<endl;
                    name_ = a;
                    cout<<"name:"<<name_<<endl;
                }
                //此时传进来的rhs是左值变量，但引用的内容是右值
                //为了将内容传递给name_，需要将rhs的右值内容通过move来获取
                //最终传入string的移动构造函数中
                A(string&& rhs):name_(std::move(rhs)) 
                {
                    cout<<"move construct"<<endl;
                    cout<<"para:"<<rhs<<endl;
                    cout<<"name:"<<name_<<endl;
                }
            private:
                string name_;
        };

        int main() {
            A a("blackwall");
            //等号左边是左值变量，右边是右值，是内容
            string s = "whitewall"; 
            //为了实现移动构造，需要将左值变量的右值内容
            //传给移动构造函数的右值引用
            A b(std::move(s));
            const string ss = "greenwall";
            A c(std::move(ss));
            cout << "Hello, World!" << endl;
            return 0;
        }

        //output
        ↪ ./a.out 
        move construct
        para:
        name:blackwall
        move construct
        para:
        name:whitewall
        copy construct
        para:greenwall
        name:greenwall
        Hello, World!
        ```

- **std::forward**的作用是当我们传入的参数是左值时，在内部将参数转发到其他函数时仍然是按照左值转发(也就是调用左值参数的函数)，而当是右值时按照右值转发(调用右值参数的函数)；仅当传入的参数被一个右值初始化过后，**std::forward**会把该参数转换成一个右值。

    ```c++
    void process(const Widget& lvalArg);
    void process(Widget&& rvalArg);

    //外部函数形参接收右值引用
    template<typename T>
    void logAndProcess(T&& param)
    {
        auto now = std::chrono::system_clock::now();
        makeLogEntry("Calling 'process'", now);
        //转发时按照形参接收的类型进行转发
        process(std::forward<T>(param));
    }

    Widget w;
    logAndProcess(w); 
    logAndProcess(std::move(w));
    ```

- **std::move**完全可以使用**std::forward**来代替，而且**std::forward**完全可以使用**static\_cast**来代替
    - 但是使用**std::forward**来代替**std::move**时，需要额外接收一个模板类型参数，且该模板参数不能是引用类型，因为编码方式决定了传递的值必须是一个右值
    - 使用**static\_cast**来代替**std::forward**时需要在每个需要的地方手动编写转换过程，这种方式不够简洁且会出错。

## 2. Distinguish universal references from rvalue references

- **T&&** 通常有两种不同意义
    - 右值引用：绑定到右值，主要作用是识别那些也许会被移动的对象
    - 通用引用：右值引用或者左值引用，可以绑定到左值或者右值，也可以绑定到**const**或非**const**对象，**volatile**或非**volatile**对象上，甚至是即**const**又**volatile**对象上。
        - 通用引用出现在两种上下文中，共同特点是上下文中有类型推导​
            - 函数模板参数
            - **auto**声明
                ```c++
                void f(Widget&& param); // 右值引用
                Widget&& var1 = Widget(); // 右值引用
                auto&& var2 = var1; // 通用引用
                template<typename T>
                void f(std::vector<T>&& param); // 右值引用
                template<typename T>
                void f(T&& param); // 通用引用
                ```
        - 传入的参数是左值，即为左值引用；右值即为右值引用

            ```c++
            template<typename T>
            void f(T&& param);

            Widget W;
            f(w); // 左值传递，左值引用

            f(std::move(w)); // 右值传递，右值引用
            ```
        - 类型推导是通用引用的必要条件，但不是充分条件
            - 首先引用声明的形式必须正确，且必须是**T&&** 类型：
                ```c++
                // 右值引用，因为声明的形式不是T&&，而是std::vector<T>&&
                template<typename T>
                void f(std::vector<T>&& param); 
                ```
            - 也不能有const修饰符
                ```c++
                template<typename T>
                void f(const T&& param); 
                ```
            - 模板类不能保证类型推导的存在
                ```c++
                template<class T, class Allocator = allocator<T>>
                class vector {
                    public:
                        //因为没有特定vector实例存在时，push_back也不会存在
                        //而实例的类型完全决定了push_back的声明
                        void push_back(T&& x); 
                        ...
                };

                //当出现如下情况时
                std::vector<Widget> v;
                //使得
                class vector<Widget, allocator<Widget>> {
                    public:
                    //右值引用,此处并没有使用类型推导
                    void push_back(Widget&& x); 
                    ...
                };

                //相反，emplace_back成员函数却使用了类型推导
                template<class T, class Allocator = allocator<T>>
                class vector {
                    public:
                    //此处类型参数Args和vector的类型参数T无关
                    //所以每次调用时都要做类型推导
                    template<class... Args>
                    void emplace_back(Args&&... args); 
                    ...
                };
                ```
            - 通用引用的基础是抽象，底层事实上是通过引用折叠来完成的。

## 3. Use std::move on rvalue references, std::forward on universal references

- 在转发右值引用时，右值引用应当无条件地被转换成右值，而通用引用应当有条件地被转换成右值仅当它们绑定到右值上时。
    - **std::move**不应该用于通用引用
        ```c++
        class Widget {
            public:
            template<typename T>
            void setName(T&& newName)  //通用引用
            {  name = std::move(newName); } //但是却使用std::move
            ...
            private:
            std::string name;
            std::shared_ptr<SomeDataStructure> p;
        };

        std::string getWidgetName(); //工厂函数

        Widget w;

        auto n = getWidgetName(); //n是一个局部变量

        //把n的值给移动走了，因为通用引用可以识别左值或右值引用
        w.setName(n); 
        ```
    - 如果通过指定左值引用和右值引用函数来代替通用引用，那么这种做法会使得手写重载函数数量因为函数参数数量而呈指数增加
- **std::move**和**std::forward**仅仅用在最后一次使用该引用的地方
    ```c++
    template<typename T>
    void setSignText(T&& text)
    {
        sign.setText(text); // 使用text但不修改它
        auto now = std::chrono::system_clock::now();
        //有条件地将text转换成右值
        signHistory.add(now, std::forward<T>(text)); 
    }
    ```
- 返回右值引用或者通用引用的函数，可以通过**std::move**或**std::forward**将值直接移动到返回值内存中
    ```c++
    Matrix operator+(Matrix&& lhs, const Matrix& rhs)
    {
        lhs+=rhs;
        //移动lhs到返回值内存中,即便Matrix不支持移动
        //也只会简单的把右值拷贝到返回值内存中
        return std::move(lhs); 
    }

    Matrix operator+(Matrix&& lhs, const Matrix& rhs)
    {
        lhs+=rhs;
        return lhs; //拷贝lhs到返回值内存中
    }

    template<typename T>
    Fraction reduceAndCopy(T&& frac)
    {
        frac.reduce();
        //如果传入的是右值，就移动返回，如果是左值，就拷贝返回
        return std::forward<T>(frac);
    }
    ```

- 对于返回局部变量的值，不能完全效仿上述规则
    ```c++
    Widget makeWidget()
    {
        Widget w;  
        ...
        return w; //“拷贝”返回
    }

    Widget makeWidget()
    {
        Widget w;
        ...
        return std::move(w); // “移动”返回
    }
    ```
    - 编译器在处理返回值的函数时会采用一种优化**Return** **Value** **Optimization(RVO)** ，它有时候会在返回值内存中直接构造这个结果。但是需要满足两个条件：
        - 函数返回类型和局部对象类型一致
        - 返回的值就是这个局部对象
    - 因此，在上述拷贝返回值的函数中，满足了上述两个条件，编译器会使用**RVO**来避免拷贝。但是针对移动返回值的函数中，编译器不会执行**RVO**，因为这个函数不满足条件**2**，也就是返回值并不是局部对象本身，而是局部对象的引用，因此，编译器只能把**w**移动到返回值的位置。这样以来，那些想要通过对局部变量使用**std::move**来帮助编译器进行优化的程序员，实际上却限制了编译器的优化选择。
    - **RVO**是一种优化方式，但是即便允许编译器避免拷贝而执行移动操作，它们也不一定会执行，因为有些场景下比如返回多种局部变量时，编译器无法确定到底返回哪一个。
    - 事实上，标准委员会要求：如果允许执行**RVO**优化，那么在返回局部变量时，要么执行复制**RVO**，要么隐式的执行**std::move**。也就是说，在拷贝返回值的函数中，**w**要么被**RVO**优化，要么实际上被执行为**return** **std::move(w)**;。这在按值传入的函数参数中也是类似，如果这些参数最后是函数的返回值，那么编译器也必须把它当做右值来处理。
        ```c++
        Widget makeWidget(Widget w)
        {
            ...
            return w;  --->  return std::move(w);
            //被编译器认为是一个右值  
        }
        ```

## 4. Avoid overloading on universal references

- 不要既重载通用引用参数的函数，又重载特定类型参数的函数，这样会造成匹配问题
    - 按照正常的重载解析规则，完全匹配会胜过类型提升匹配，在这种情况下，通用引用重载函数会被调用
        ```c++
        std::multiset<std::string> names; //全局数据结构
        std::string nameFromIdx(int idx); //根据索引返回姓名字符串

        template<typename T>
        void logAndAdd(T&& name)
        {
            auto now = std::chrono::system_clock::now();
            log(now, "logAndAdd");
            names.emplace(std::forward<T>(name));
        }

        void logAndAdd(int idx)
        {
            auto now = std::chrono::system_clock::now();
            log(now, "logAndAdd");
            names.emplace(nameFromIdx(idx));
        }

        std::string petName("Darla"); 
        logAndAdd(petName); //拷贝左值
        logAndAdd(std::string("Persephone")); //移动右值
        //直接在multiset中创建string而不是拷贝一个临时字符串
        logAndAdd("Patty Dog"); 
        short nameIdx = 22; 
        //错误，short参数将会匹配到通用引用参数的函数调用
        //在将short参数转发到names的string构造函数中时，会出错
        logAndAdd(nameIdx); 
        ```
    - 带有通用引用参数的函数是**C++** 中最贪婪的函数，它们几乎对所有类型的参数都会产生完美匹配的实例化。这样它就会产生许许多多的参数类型的重载实例函数。
- 在编译器为类自动生成移动和拷贝构造函数时，也不能使用重载过的通用引用参数构造函数，因为通用引用参数的构造函数在匹配顺序上会在其他重载函数之前。
    ```c++
    class Person{
        public:
        template<typename T>
        explicit Person(T&& n):name(std::forward<T>(n)) {}
        explicit Person(int idx):name(nameFromIdx)) {}
        Person(const Person& rhs); //编译器自动产生
        Person(Person&& rhs); //编译器自动产生
        ...
        private:
        std::string name;
    };

    Person p("Nancy");
    auto cloneOfP(p); //出错！！！
    ```
    - 在合适的条件下，即便存在模板构造函数可以通过实例化来产生拷贝或者移动构造函数，编译器也会自动产生拷贝或者移动构造函数。
    - 上述**auto** **cloneOfP(p)** 语句似乎应该是调用拷贝构造函数，但是实际上会调用完美转发构造函数，然后会用**Person**对象去实例化**Person**的**string**成员，然而并没有这种匹配规则，马上报错！
    - 如果对传入的对象**p**加上**const**修饰，那么虽然模板函数虽然会被实例化成为一个接收**const**类型**Person**对象的函数，但是具有在**const**类型参数的所有重载函数中，**C++** 中的重载解析规则是：当模板实例函数和非模板函数同样都能匹配一个函数调用，那么非模板函数的调用顺序优先模板函数。

## 5. Familiarize yourself with alternatives to overloading on universal references

- 三种简单的方法来代替对通用引用的重载
    - 针对上面提到的**logAndAdd**类型，可以放弃重载，使用独立的函数**logAndAddName**和**logAndAddNameIdx**来分别实现，但是这个针对构造函数的通用引用参数情况无法适用，因为构造函数的名字是固定的。
    - 通过传递**const**左值引用，缺点是效率不高
    - 通过传值的方式
        ```c++
        class Person{
            public:
            explicit Person(std::string n):name(std::move(n)) {}
            explicit Person(int idx):name(nameFromIdx)) {}
            ...
            private:
            std::string name;
        };
        ```
        - 这样以来，构造函数不仅能正确匹配，而且可以使用移动语义将拷贝传递的参数直接移动给成员变量。
- 一种高级做法，使用标签分发方式**(Tag dispatch)**
    - 传递**const**左值引用和传值方式都不支持完美转发，如果使用通用引用是为了完美转发，那就不得不使用通用引用，同时如果不想放弃重载，就需要在特定条件下强制模板函数匹配无效。
    - 在调用点解析重载函数具体是通过匹配调用点的所有参数与所有重载函数的参数进行匹配实现的。通用引用参数一般会对任何传入的参数产生匹配，但是如果通用引用是包含其他非通用引用参数的参数列表中的一部分，那么在非通用引用参数上的不匹配会使得已经匹配的通用引用参数无效。这就是标签分发的基础。
        ```c++
        //标签分发函数，通过使用对参数类型的判断
        //使得通用引用参数获得的匹配无效
        //将控制流分发到两个不同的处理函数中
        template<typename T>
        void logAndAdd(T&& name) 
        {
            //此处必须去掉引用，因为std::is_integral会把int& 判断为
            //非int类型，也就是std::false_type
            logAndAddImpl(
                std::forward<T>(name), 
                std::is_integral<typename std::remove_reference<T>::type>()
            );
        }

        //处理非整型参数
        template<typename T>
        void logAndAddImpl(T&& name, std::false_type) 
        {
            auto now = std::chrono::system_clock::now();
            log(now, "logAndAdd");
            names.emplace(std::forward<T>(name));
        }

        //处理整型参数类型
        std::string nameFromIdx(int idx);
        void logAndAddImpl(int idx, std::true_type) 
        {
            //将整型转换成字符串，再重新转发到标签分发函数中，再次分发
            logAndAdd(nameFromIdx(idx)); 
        }
        ```
        - 上面的**std::true\_type**和**std::false\_type**就是标签，我们可以利用它们来强制选择我们希望调用的重载函数，这在模板元编程中非常常见。这种做法的核心是存在一个未重载过的函数作为客户端的**API**，然后将任务分发到其他实现函数中。但是，这种做法针对类的构造函数不可行，因为即便将构造函数写成标签分发函数，在其他函数中完成具体的任务，但是有些构造调用也会绕过标签分发函数而转向编译器自动生成的拷贝和移动构造函数。问题在于传入的参数并不总是会匹配到通用引用参数的函数，尽管大多数情况下确实会匹配。
- 另一种高级做法，限制**(constraining)** 采用通用应用的模板
    - 为了在特定的条件下，让函数调用发生在应该发生的位置上，我们需要根据条件来启用/禁用模板匹配，方式是**std::enable\_if**，如果内部判断条件为**true**，那么就会启用模板，否则会禁用模板
        ```c++
        class Person{
            public:
            //在condition中指定满足什么条件
            template<typename T, 
                typename = typename std::enable_if<condition>::type>　
            explicit Person(T&& n);
            ...
        };
        ```
    - 在这种例子下，我们想要的结果是：当传入的参数类型是**Person**时，应该调用拷贝构造函数，也就是要禁用模板；否则应该启用模板，将函数调用匹配到通用引用构造函数中。判断类型的方式是**std::is\_same**
        ```c++
        class Person{
            public:
            template<typename T, 
                typename = typename std::enable_if<!std::is_same<Person, T>::value>::type>
            explicit Person(T&& n);
            ...
        };
        ```
        - 但是这中间有一个问题，就是**std::is\_same**会把**Person**和**Person&**判断为不同类型，因此我们希望会略掉对这个**Person**类型的一切修饰符，拿到最原始的类型，这需要用到**std::decay<T>::type**
            ```c++
            //无论是否是引用: Person , Person& , Person&&应该和Person一样
            //无论是否是const或volatile: const Person , volatile Person 
            //const volatile Person应该和Person一样
            class Person{
                public:
                template<typename T, typename = typename std::enable_if<!std::is_same<Person, typename std::decay<T>::type>::value>::type>
                explicit Person(T&& n);
                ...
            };
            ```
    - 但是上面的做法在有派生类存在的情况下会出现问题
        ```c++
        class SpecialPerson: public Person{
            public:
            //子类构造函数应该先调用父类构造函数，但是传入的参数类型是
            //SpecialPerson，根据上面的类型判断，Person与SpecialPerson
            //不是同一个类型，因此会禁用模板函数，转而调用拷贝构造函数
            SpecialPerson(const SpecialPerson& rhs):
            Person(rhs) {...} 
            SpecialPerson(SpecialPerson&& rhs): 
            Person(std::move(rhs)) {...}
            ...
        };
        ```
        - 因此，我们需要一个能够判断一个类型是继承自另一个类型的方法，这就是**std::is\_base\_of<T1,T2>::value**，这种方法在**T2**是**T1**的子类时返回**true**。对于用户自定义的类型而言，他们是继承自自身的，也就是说**std::is\_base\_of<T,T>**会返回为**true**，但是当**T**是内建类型时，就会返回为**false**。
            ```c++
            class Person{
                public:
                template<typename T, 
                typename = typename std::enable_if<
                    !std::is_base_of<
                    Person, typename std::decay<T>::type
                                    >::value
                    >::type
                >
                explicit Person(T&& n);
                ...
            };
            ```
        - 在加上之前标签分发的方式，就是如下形式
            ```c++
            class Person{
                public:
                template<typename T, typename = typename std::enable_if<!std::is_base_of<Person, typename std::decay<T>::type>::value && !std::is_integral<std::remove_reference<T>::type>()>::type>
                explicit Person(T&& n): name(std::forward<T>(n)) {...}
                explicit Person(int idx): name(nameFromIdx(idx)) {...}
                ...
                private:
                std::string name;
            };
            ```

- **Trade-offs**
    - 在完美转发情况下，假设对**Person**传入的是字符串文本值"**Nancy**"，就会直接把这个值转发到内部。
    - 在普通情况下，假设对**Person**传入的是字符串文本值"**Nancy**"，会先把**Nancy**构造成一个临时**std::string**对象，然后在传入构造函数内部。
    - 但是问题是，完美转发在有些情况下参数不能被完美转发。
    - 另一个问题是出现错误时，错误信息的易理解性，因为完美转发不会做参数类型是否符合最内层函数的类型，如果中间经过许多层转发，那么最后如果出现类型不匹配的错误，就会输出大量的错误信息，此时需要在适当的位置做一次预先判断，如下
        ```c++
        class Person{
            public:
            template<typename T, typename = typename std::enable_if<!std::is_base_of<Person, typename std::decay<T>::type>::value && !std::is_integral<std::remove_reference<T>::type>()>::type>
            explicit Person(T&& n): name(std::forward<T>(n)) 
            {
                //因为该函数在转发之后执行
                //因此这条错误信息将会在左右错误信息输出之后出现
                static_assert(std::is_constructible<std::string, T>::value, "Parameter n can't be used to construct a std::string");　
            }
            ...
            private:
            std::string name;
        };
        ```

## 6. Understand reference collapsing

- 当模板函数的参数是一个通用引用参数时，当一个参数传递给这个模板函数，模板参数推导的类型才会编码这个参数是左值还是右值。
- 编码机制是：当传递的参数是一个左值时，模板参数被推导为左值引用；当传递的参数是一个右值时，模板参数被推到为一个非引用。
    ```c++
    template<typename T>
    void func(T&& param);

    Widget widgetFactory();
    Widget w;
    func(w); // T被推导为Widget&
    func(widgetFactory()); //T被推导为Widget
    ```

- **C++** 不允许用户使用指向引用的引用，但是编译器编译出的结果中如果出现了多重引用，就会应用引用折叠
    ```c++
    int x;
    ...
    auto& & rx = x; // 错误

    template<typename T>
    void func(T&& param);

    Widget w;
    func(w); // T被推导为Widget&
    ```​
    - 如果拿推导出的类型**Widget&**再去引用到**func**模板上，就会出现**void** **func(Widget&** **&¶m)**; 我们知道当通用引用参数被一个左值初始化，那么这个参数的类型就应该是左值引用。由此可知，这里发生了引用折叠。
- 因为存在两种引用类型(左值引用和右值引用)，因此就有四种可能的引用到引用的组合，即：指向左值引用的左值引用，指向右值引用的左值引用，指向左值引用的右值引用，指向右值引用的右值引用。折叠引用的规则如下：
    - 如果有一个引用是左值引用，那么结果就是左值引用。
    - 否则，结果就是右值引用，比如两个都是右值引用时。
    - 简化版的**std::forward**
        ```c++
        template<typename T>
        T&& forward(typename remove_reference<T>::type& param)
        {
            return static_cast<T&&>(param);
        }
        ```
    - 那么把各种情况应用到**std::forward**函数上时，就有如下结果(引用折叠出现的第一种环境：模板实例化)
        ```c++
        template<typename T>
        void f(T&& fParam)
        {
            ...
            someFunc(std::forward<T>(fParam);
        }

        1.传入左值
        Widget w;
        f(w);
        ------->
        void f(Widget& fParam)
        {
            ...
            someFunc(std::forward<Widget&>(fParam);
        }

        Widget& && forward(typename remove_reference<Widget&>::type& param)
        {
            return static_cast<Widget& &&>(param);
        }

        --->
        Widget& && forward(Widget& param)
        {
            return static_cast<Widget& &&>(param);
        }

        ->
        Widget& forward(Widget& param)
        {
            return static_cast<Widget&>(param);
        }

        2.传入右值
        Widget w;
        f(std::move(w));
        ------->
        void f(Widget fParam)
        {
            ...
            someFunc(std::forward<Widget>(fParam);
        }

        Widget && forward(typename remove_reference<Widget>::type& param)
        {
            return static_cast<Widget&&>(param);
        }

        --->
        Widget&& forward(Widget& param)
        {
            return static_cast<Widget&&>(param);
        }
        ```

- 引用折叠出现的第二种环境是**auto**变量的类型推导
    ```c++
    Widget widgetFactory();

    Widget w;

    //发生了引用折叠
    auto&& w1 = w;  --> Widget& && w1 = w; --> Widget& w1 = w;
    //没有发生引用折叠
    auto&& w2 = widgetFactory(); -->Widget&& w2 = widgetFactory();
    ```
    - 可以看出，通用引用并不是一种新的引用，实际上是一种满足两种条件的右值引用：１．类型推导区分左值引用和右值引用 ２．发生引用折叠
- 其他两种会出现引用折叠的环境是：
    - 使用**typedef**和**alias**声明
        ```c++
        template<typename T>
        class Widget{
            public:
            typedef T&& RvalueRefToT;
            ...
        };

        Widget<int&> w; --> typedef int& && RvalueRefToT; --> typedef int& RvalueRefToT;
        ```
    - 使用**decltype**的地方

## 7. Assume that move operations are not present, not cheap, and not used

- 不能支持移动语义的类型
    - 禁用移动操作的数据成员或者基类类型
    - 没有显式支持移动操作或不满足编译器自动生成移动操作的类型
    - 并非所有支持移动的标准库容器都会受益于移动操作
        - 对于把内容存储在堆内存中，而自身只保存指向该堆内存指针的容器类型来说，移动操作仅仅是拷贝这个指针到新的容器中，将原来的容器指针置空。
        - **std::array**没有这个特性，因为它把内容存储在自身空间中，即便存储的内容对象本身支持移动操作，且移动操作比拷贝要快，而且**std::array**也支持移动操作，但对于**std::array**来说，移动操作和拷贝操作代价一样，都是线性时间复杂度，因为容器中每个元素必须被拷贝或者移动。
        - **std::string**虽然也提供常量时间的移动操作和线性时间的拷贝操作，但是并不总是这样。因为许多实现中会存在一种**small** **string** **optimization(SSO)** 的优化。对于长度较短的字符串，它们会被存储在**std::string**对象自身的空间中，而不是新开辟一块堆内存来存放内容，在这种情况下，移动和拷贝的代价将会是一样的。
    - 没有声明异常安全的移动操作，也会被替换成拷贝操作
    - 在少数例外情况下，只有右值才能被移动，而如果源对象是左值，就不能被移动８．Familiarize yourself with perfect forwarding failure cases
- 完美转发失败的直观解释
    ```c++
    template<typename T>
    void fwd(T&& param)
    {
        f(std::forward<T>(param));
    }

    template<typename... Ts>
    void fwd(Ts&&... params)
    {
        f(std::forward<Ts>(params)...);
    }
    ```
    - 如果调用**fwd**和直接调用**f**函数所造成的结果不同，那么就是说完美转发出现了问题
- 花括号初始化会使完美转发失败
    - 比如：
        ```c++
        void f(const std::vector<int>& v);

        f({1,2,3}); //会被隐式转换成std::vector<int>

        fwd({1,2,3}); //无法编译 
        ```
    - 原因是：
        - 直接调用**f**的时候，编译器可以看到在调用点传递的参数，以及函数**f**定义的参数类型，然后比较他们是否兼容，如果有必要，就执行隐式转换
        - 通过完美转发间接调用**f**的时候，编译器就不会对在**fwd**调用点传递的参数和**f**声明的参数进行比较；而是会拿从**fwd**推导的参数类型和函数**f**的参数声明进行比较。而标准规定：向函数模板传递一个花括号初始化的参数，而模板参数又没有指定参数类型为**std::initializer\_list**，那么这就是一个不可推导的情况。
    - 这种情况下出错的类型有：
        - 编译器无法推导出一个类型：只要参数中有一个及以上无法推导出类型，就无法编译
        - 编译器推到出错误的类型：要么是推导出来的类型使得无法编译，要么是推到出来的类型在重载函数情况下匹配到错误的函数调用。
    - 在例子中，正确的做法应该是
        ```c++
        auto il = {1,2,3};
        fwd(il);
        ```
        - 因为，花括号初始化对于**auto**变量的类型推导是可以被推导成**std::initializer\_list**对象的，而有了具体类型之后，在模板函数中就可以进行匹配。
- 把**0**或**NULL**当做空指针传入的时候，完美转发也会失败
    - 因为推导的时候会把这两个值推导为**int**型
    - 正确做法应该是传入**nullptr**
- 传递只有声明的整型**static** **const**和**constexpr**数据成员，完美转发会失败
    - 例子：
        ```c++
        class Widget {
            public:
                static constexpr std::size_t MinVals = 28;
                ...
        };
        ...

        std::vector<int> widgetData;
        widgetData.reserve(Widget::MinVals); //没问题

        void f(std::size_t val);

        f(Widget::MinVals); --> f(28); //没问题

        fwd(Widget::MinVals); //出错
        ```
    - 因为对于缺乏定义的**MinVals**，编译器会把用到此值的地方替换成**28**，而不用分配内存，但是如果要取地址的话，编译器就会分配一块内存来存储这个值，并返回内存的地址，不提供定义这种做法只能在编译期通过，在链接的过程就会报错。同样，在将**MinVals**传递到模板函数**fwd**中时，这个模板参数是一个引用，它本质上和指针是一样，只不过是一个会自动解引用的指针，那么在编译该函数时就需要对**MinVals**进行取地址，而**MinVals**此时并没有定义，也就没有内存空间。
    - 但是上述行为实际上是依赖于编译器的，安全的做法是在**cpp**文件中定义一次**MinVals**
        ```c++
        constexpr std::size_t Widget::MinVals;
        ```
- 重载函数名和模板名的自动推导
    - 一个模板函数接收重载函数作为参数时，模板函数无法自动推导出用户想要调用的重载函数
        ```c++
        template<typename T>
        void fwd(T&& param)
        {
            f(std::forward<T>(param));
        }

        void f(int (*pf)(int)); / void f(int pf(int));

        int processVal(int value);
        int processVal(int value, int priority);

        //虽然processVal不是一个类型
        //但编译器可以正确匹配到第一个重载函数
        f(processVal); 

        //错误，proecssVal不是一个类型
        //自动推导的fwd不知道该匹配哪一个重载函数
        fwd(processVal); 
        ```
    - 如果将模板函数作为模板函数的参数，同样也无法自动推导出匹配的函数，因为模板函数不是一个函数，而是许多函数
        ```c++
        template<typename T>
        T workOnVal(T param) {...}
        //出错，不知道匹配哪一个模板函数实例
        fwd(workOnVal); 
        ```
    - 正确的做法是声明一个具体的函数签名，存储到一个函数指针变量中，然后再传递给模板函数
        ```c++
        using ProcessFuncType = int (*)(int);
        ProcessFuncType processValPtr = processVal; //指定函数
        fwd(processValPtr); //可以正确转发，因为类型已经指定

        //可以正确转发，因为已经实例化
        fwd(static_cast<ProcessFuncType>(workOnVal); 
        ```
- Bitfields的通用引用识别也会使得完美转发出错
    - 例子：
        ```c++
        struct IPv4Header {
            std::uint32_t version:4, IHL:4, DSCP:6, ECN:2, totalLength:16;
            ...
        };

        void f(std::size_t sz);

        IPv4Header h;

        //没问题，bit会自动转换成size_t
        f(h.totalLength); 

        //出错，因为fwd的参数是一个通用引用
        //而C++标准规定：非const类型的引用不能绑定到bit域上
        //因为没有办法寻址
        fwd(h.totalLength);

        //bit域参数传递的可行方式只有：按值传递，或者加上const修饰的引用。
        //按值传递时，函数会接收到bit域里面的值
        //按const引用传递时，会首先将bit域的值拷贝到一个整型类型中，
        //然后再绑定到该类型上

        auto length = static_cast<std::uint16_t>(h.totalLength);

        fwd(length); 
        ```

## 9. Summary

- **std::move performs an unconditional cast to an rvalue. In and of itself, it doesn't move anything.**
- **std::forward casts it argument to an rvalue only if that argument is bound to an rvalue.**
- **Neither std::move nor std::forward do anything at runtime.**
- **If a function template parameter has type T&& for a deduced type T, or if an object is declared using auto&&, the parameter or object is a universal reference.**
- **If the form of the type declaration isn't precisely type&&, or if type deduction doesn't occur, type&& denotes an rvalue reference.**
- **Universal references correspond to rvalue references if they're initialized with rvalues. They correspond to lvalue references if they're initialized with lvalues.**
- **Apply std::move to rvalue references and std::forward to universal references the last time each is used.**
- **Do the same thing for rvalue references and universal references being returned from functions that return by value.**
- **Never apply std::move or std::forward to local objects if they would otherwise be eligible for the return value optimization.**
- **Overloading on universal references almost always leads to the universal reference overloading being called more frequently than expected.**
- **Perfect-forwarding constructors are especially problematic, because they're typically better matches than copy constructors for non-const lvalues, and they can hijack derived class calls to base class copy and move constructors.**
- **Alternatives to the combination of universal references and overloading include the use of distinct function names, passing parameters by lvalue-reference-to-const, passing parameters by value, and using tag dispatch.**
- **Constraining templates via std::enable\_if permits the use of universal references and overloading together, but it controls the conditions under which compilers may use the universal reference overloadings.**
- **Universal reference parameters often have efficiency advantages, but they typically have usability disadvantages.**
- **Reference collapsing occurs in four contexts: template instantiation, auto type generation, creation and use of typedefs and alias declarations, and decltype.**
- **When compilers generate a reference to a reference in a reference collapsing context, the result becomes a single reference. If either of the original references is an lvalue reference, the result is an lvalue reference. Otherwise it's an rvalue reference.**
- **Universal references are rvalue references in contexts where type deduction distinguishes lvalues from rvalues and where reference collapsing occurs.**
- **Assume that move operations are not present, not cheap, and not used.**
- **In code with known types or support for move semantics, there is no need for assumptions.**
- **Perfect forwarding fails when template type deduction fails or when it deduces the wrong type.**
- **The kinds of arguments that lead to perfect forwarding failure are braced initializers, null pointers expressed as 0 or NULL, declaration-only integral const static data members, template and overloaded function names, and bit fields.**
