## 1.更多的使用auto而不是显式类型声明

- 将大段声明缩减成**auto**
    ```c++
    typename std::iterator_traits<It>::value_type currValue = *b;
    auto currValue = *b;
    ```

- 使用**auto**可以防止变量未初始化
    ```c++
    int x1; //正确，但是未初始化
    auto x2; //错误，没有初始化
    auto x3 = 3; //正确，声明并初始化
    ```

- 在模板函数中可以使用**auto**来完成变量的自动类型推导
    ```c++
    template<typename It>
    void dwim(It b, It e)
    {
        for(; b!=e; ++b)
        {
            auto currValue = *b;
            ...
        }   
    }
    ```

- 使用**auto**来自动推断**lambda**表达式的返回结果类型
    ```c++
    auto derefLess = [](const std::unique_ptr<Widget>& p1, const std::unique_ptr<Widget>& p2){ return *p1 < *p2; };
    ```
    - 相比之下，使用**function**对象来保存上述结果，代码如下：
        ```c++
        std::function<bool(const std::unique_ptr<Widget>&, const std::unique_ptr<Widget>& )> 
        　　               derefUPLess = [](
            const std::unique_ptr<Widget>& p1, const std::unique_ptr<Widget>& p2){ return *p1 < *p2; };
        ```
    - 这两种表达形式的区别是：
        - **auto**声明的变量，占用和**lambda**表达式同样大的内存空间
        - 使用**std::function**声明的变量对于任何函数都是固定大小的空间，如果空间不足，就会在堆上申请内存来存储这个闭包。另外，由于限制内联，函数对象不得不产生一次间接函数调用。
        - 结果是：**std::function**对象通常使用更多的内存，执行速度也比**auto**要慢。
- 使用**auto**来避免"**type** **shortcuts**"
    - 例如：
        ```c++
        std::vector<int> v;
        ...
        unsigned sz = v.size();// v.size()返回值类型是 std::vector<int>::size_type, 指定是一种无符号整型类型
        ```
    - 上述代码在**32**位**windows**上，**unsigned**和**std::vector<int>::size\_type**都是**32**位，但是在**64**位**windows**上，**unsigned**是**32**位而**std::vector<int>::size\_type**是**64**位，因此在不同的机器上运行相同的代码可能会出错，这种与底层系统耦合性较强的错误不应该出现。
    - 因此，正确的用法如下：
        ```c++
        auto sz = v.size(); 
        ```
- 使用**auto**声明变量来避免类型不匹配时的隐式转换带来的额外代价
    ```c++
    std::unordered_map<std::string, int> m;
    ...
    for(const std::pair<std::string, int>& p : m)
    {
        ... // do somethins with p   
    }
    ```
    - 上述代码的毛病在于：**std::unordered\_map**的**key**部分是**const**属性，因此哈希表中的**std::pair**类型实际上应该是**std::pair<const** **std::string,** **int>**。但是上述循环中声明的是一个**const** **std::pair<std::string,** **int>**，因此编译器不得不在这两种类型中做一个转换，首先为了得到一个**std::pair<std::string,** **int>**，编译器需要从**m**中对每个对象进行一次拷贝，创建一系列临时变量，然后再将这些临时变量依次绑定到引用**p**，在循环结束时，这些临时变量再被编译器进行销毁。
    - 正确的做法应该是：
        ```c++
        for( const auto& p : m)
        {
            ... // as before
        }
        ```
- 有关代码可读性的建议：
    - 如果使用显示类型声明使得代码更清晰且更容易维护，那么就应该使用显示类型声明，否则就应该试着使用**auto**
    - 通过**auto**声明的变量，如果想要方便获取是什么类型，可以通过命名规则来间接表示类型。

## 2.当auto推导出错误类型时使用显式类型初始化方式

- 当表达式返回的类型是代理类的类型时，不能使用**auto**
    ```c++
    //提取出Widget对象的特征，并以vector<bool>的形式返回
    //每一个bool代表该feature是否存在
    std::vector<bool> features(const Widget& w);

    Widget w;
    ...
    //访问特定的feature标志位5
    bool highPriority = features(w)[5];//(1)
    auto highPriority_alt = features(w)[5];//(2)
    ...
    //根据上述标志位的值来处理Widget对象
    processWidget(w, highPriority);//(3)
    processWidget(w, highPriority_alt);//(4)
    ...
    ```
    - 上述代码中(1)(3)可以正常运行，但是(2)(4)就会出现未定义行为，这是为什么?
        - 因为**std::vector<bool>**虽然持有**bool**，但是**operator**[]作用于**vector<bool>**时，并不会返回**vector**容器中元素的引用([]操作返回容器内元素的引用对于其他类型都适用，唯独**bool**不适用)，而是返回一个**std::vector<bool>::reference**类型的对象。为什么会存在这种类型的对象呢？因为**vector<bool>**是通过紧凑的形式来表示**bool**值，每一个**bit**代表一个**bool**。这给[]操作造成了困难，因为对于**std::vector<T>**，[]操作理应返回的是一个**T&**对象，但是**C++**禁止返回对**bit**的引用，也就是不能返回**bool&**，那么就得想办法返回一个对象来模拟**bool&**的行为。因此，**std::vector<bool>::reference**对象就出现了，它可以在需要的地方自动从**bool&**转换成**bool**类型。所以，在(1)中，隐式自动转换是成功的，而在(2)中，**auto**自动接收了**std::vector<bool>::reference**对象的类型，没有发生转换，而该对象实际指向的是一个临时**std::vector<bool>**对象的内部内存地址，当执行完这条语句后，该临时对象被销毁，那么**auto**保存的地址也就属于悬空地址。在(4)中就会出发未定义行为。
    - 代理介绍
        - **std::vector<bool>::reference**是代理类的一个例子，它们存在的目的是模拟和增强其他类型的行为。例如标准库中智能指针类型也是代理类的例子，它们负责对原始指针指向资源的管理。
        - 有一些代理类是对用户可见的，比如**std::shared\_ptr,std::unique\_ptr**。而另一些代理类则是用户不可见的，比如**:** **std::vector<bool>::reference**和**std::bitset::reference**。
        - **C++**某些库中使用的叫做表达式模板的技术也属于这个范畴，这种库是为了改善数值计算代码的效率。
            ```c++
            Matrix m1, m2, m3, m4;
            ...
            Matrix sum = m1 + m2 + m3 + m4;
            ```
        - 如果**operator+**操作返回的是一个代理类比如**:Sum<Matrix,** **Matrix>**而不是结果本身也就是**Matrix**对象，那么这样就可以高效计算这个表达式。
        - 因为对象的类型会被编码到整个初始化表达式，比如：**Sum<Sum<Sum<Matrix,** **Matrix>,** **Matrix>,** **Matrix>**。
    - 一般性规则，不可见的代理类不适用与**auto**，因为代理类对象一般只存活于一条语句内，因此创建代理类对象的变量违反了基本库设计假设。
- **auto**推到出代理类类型时，需要对表达式做代理类类型到实际类型的静态转换，而不是弃用**auto**
    ```c++
    auto highPriority = static_cast<bool>(features(w)[5]);
    ```
    - 针对上面的例2：
        ```c++
        auto sum = static_cast<Matrix>(m1 + m2 + m3 + m4);
        ```

## 3.总结

**auto**自动类型推导可以精简代码，避免隐式转换带来开销，同时增强程序可移植性和减少重构复杂性；但也由于与隐式代理类的冲突，造成了一些潜在问题，但是这些问题不是**auto**引起的，而是代理类本身的问题，因此显式静态类型转换可以保留**auto**的优点，同时保证程序的正确性。
