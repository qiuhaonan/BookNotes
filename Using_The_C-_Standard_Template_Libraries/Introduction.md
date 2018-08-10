# 1. STL介绍
## 1.1 STL划分
STL是一个功能强大且可扩展的工具集，用来组织和处理数据，可以被划分为4个概念库：
- 容器库
  - 定义了管理和存储数据的容器
  - 头文件包括： array, vector, stack, queue, deque, list, foward_list, set, unordered_set, map 和 unordered_map
- 迭代器库
  - 定义了迭代器，通常被用于引用容器类的对象序列
  - 头文件包括： iterator
- 算法库
  - 定义了可以运用到容器中的一组元素上的通用算法
  - 头文件包括： algorithm
- 数值库
  - 定义了一些通用的数学函数和一些对容器元素进行数值处理的操作
  - 头文件包括： complex, cmath, valarray, numeric, random, ratio 和 cfenv
  
## 1.2 模板
模板是一组函数或类的参数实现，因此模板并不是可执行代码，而是用于生成代码的蓝图或配方。
- 在程序中，如果一个模板从来没有被使用过，那么它将会被编译器忽略，不会生成可执行代码。
- 一个没有被使用的模板可能会包含一些编程错误，并且包含这个模板的程序仍然可以编译和运行，当模板用于生成代码时，也就是当它被编译时，模板中的错误才会被编译器标识出来。

## 1.3 容器
三种类型的容器：
- 序列容器： 以线性组织的方式存储对象，但不一定需要连续的存储空间。
- 关联容器： 存储了一些和键关联的对象，可以通过一个相关联的键从关联容器中获取对应的值，也可以通过迭代器从关联容器中得到对象。
- 容器适配器： 提供了替换机制的适配类模板，可以用来访问基础的序列容器或关联容器。

## 1.4 迭代器
迭代器是一个行为类似于指针的模板类对象。使用STL算法时，迭代器将算法和不同类型容器的元素联系了起来。
迭代器分类有：
- 输入迭代器：提供对对象的只读访问，可以后向遍历
- 输出迭代器：提供对对象的只写访问，可以后向遍历
- 正向迭代器：提供对对象的读写访问，可以后向遍历
- 双向迭代器：提供对对象的前向和后向读写访问
- 随机访问迭代器：提供对对象的随机前向和后向读写访问

流迭代器：在流和可以通过迭代器访问的数据源之间传输文本模式的数据，流迭代器不能用来传输混合类型的数据。
```c++
#include<iostream>
#include<numeric>
#include<iterator>

using namespace std;

int main()
{
    cout << "Enter numeric values separated by spaces and enter ctrl+D(linux) / ctrl+Z(windows) to end:" << endl;
    int sum = accumulate(istream_iterator<double>(cin), istream_iterator<double>(), 0.0);
    cout << "\nThe sum of the values you entered is " << sum << endl;
    return 0;
}
```
迭代器适配器：是一个类模板，为标准迭代器提供了一些特殊的行为，使它们能够从迭代器模板得到派生。定义了三种不同的迭代器：
- 反向迭代器： reverse_iterator
  - 双向反向迭代器(rbegin,rend)
  - 随机反向迭代器(rbegin rend)
- 插入迭代器： insert_iterator
  - 后向插入迭代器(back_inserter -> push_back)
  - 前向插入迭代器(front_inserter -> push_front)
  - 随机插入迭代器(inserter -> insert)
- 移动迭代器： move_iterator (make_move_iterator(begin(), end()))

## 1.5 迭代器上的运算：
- advance
  - 第一个参数是迭代器，第二个参数是整数值n， 移动迭代器n个位置
  - 当迭代器为双向迭代器或随机访问迭代器，第二个参数也可以是负数
- distance
  - 返回两个迭代器之间的元素个数
- next
  - 第一个参数是迭代器，第二个参数是整数值n， 获得迭代器正向偏移n之后的迭代器
  - 要求迭代器具有正向迭代器能力
- prev
  - 第一个参数是迭代器，第二个参数是整数值n， 获得迭代器反向偏移n之后的迭代器
  - 要求迭代器具有双向迭代器能力

## 1.6 智能指针
智能指针是一个可以模板原生指针的模板类。
与原生指针的区别是：
- 只能用来保存在堆上分配的内存的地址
- 不能对智能指针进行自增或自减这样的算数运算
智能指针的分类有：
- unique_ptr<T>: 维持对一个对象的独占权
  - unique_ptr<T>不能被拷贝，只能通过移动或生成的方式来在容器中存放它们
  - 调用reset函数可以主动重置并析构它所指向的对象，原生指针被替换为空指针；或者传递一个新对象的地址给reset函数，在析构完旧对象之后，原生指针被替换为新对象的地址值
  - 调用release函数可以主动释放所指向的对象并将原生指针替换为空指针，不必释放该对象的内存
  - 两个unique_ptr<T>或者一个unique_ptr<T>与空指针可以直接进行比较，实际上调用了get()函数来间接进行比较
  - unique_ptr<T>可以被隐式地转换为布尔值
- shared_ptr<T>: 维持对一个共享对象和引用计数
  - 只能通过拷贝构造函数或赋值运算符来复制一个shared_ptr<T>对象
  - 如果将一个空指针传递给一个shared_ptr<T>对象，那么它的地址值会变为空，同样也会使指针所指向对象的引用计数减1
  - 调用无参reset函数与传入空指针效果相同
  - 调用有参reset函数可以改变共享指针指向的对象
  - 两个shared_ptr<T>或者一个shared_ptr<T>与空指针可以直接进行比较
  - shared_ptr<T>可以被隐式地转换为布尔值
- weak_ptr<T>: 可以从shared_ptr<T>中创建，但不会增加shared_ptr<T>的引用计数
  - 主要是为了避免shared_ptr<T>的循环引用问题
  - 调用函数expired()来判断指向的shared_ptr<T>是否还存在

## 1.7 算法
STL算法分类：
- 非变化序列运算
  - 例如：inner_product() 和 accumulate()
- 可变序列运算
  - 例如：swap(), copy(), transform(), replace(), remove(), reverse(), rotate(), fill(), shuffer(), sort(), stable_sort(), binary_search(), merge(), min(), max()

## 1.8 将函数作为参数传入
- 函数指针
  - 函数名
- 函数对象
  - 一种重载了函数调用运算符operator()()的类对象
  ```c++
  #include<iostream>
  using namespace std;
  class Volume
  {
    public:
      double operator()(double x, double y, double z) { return x * y * z;}
  };
  
  int main()
  {
    Volume v;
    double room { v(1,2,3)};
    cout<<room<<endl;
    return 0;
  }
  ```
- lambda表达式
  - 定义了一个匿名函数，可以捕获并使用它们作用域内的变量
  - 给lambda表达式命名
  ```c++
  auto cube = [](double value) -> double { return value * value * value;};
  ```
  - 将lambda表达式传递给函数
  ```c++
  template<typename ForwardIter, typename F>
  void change(ForwardIter first, Forward last, F fun)
  {
    for(auto iter = first; iter != last; ++iter)
      *iter = fun( *iter );
  }

  int data[] = {1,2,3,4};
  change(std::begin(data), std::end(data), [](int value){ return value*value; });
  ```
