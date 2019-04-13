# 2. 使用序列容器
五种序列容器：array<T>, vector<T>, deque<T>, list<T>, forward_list<T>
## 2.1 使用array<T,N>容器
array<T,N>
- 定义了一种相当于标准数组的容器类型
- 不能增加或删除元素
- at()越界判断
- 不传递参数不会自动初始化，或者使用初始化列表来初始化
访问元素
- 索引表达式
- 使用at成员函数防止越界
- front()和back()访问首尾元素
- 模板函数std::get<n>()来获取容器的第n个元素，n必须是编译期可以确定的常量表达式
- 使用迭代器： begin(), end(), cbegin(), cend() 或者 std::begin(), std::end(), std::cbegin(), std::cend()
```c++
std::array<unsigned int, 100> height;
std::generate(std::begin(height), std::end(height), [](unsigned int& i) { i = 0; });
```
比较数组容器
- 数组容器大小相同， 元素类型相同， 元素要支持比较运算符
- 容器被逐元素地比较
只要两个容器存放的是相同类型、相同个数的元素，就可以将一个数组容器赋值给另一个，而标准数组不能赋值

## 2.2 使用vector<T>容器
vector<T>
- 大小自动增长
- 调用reserve()函数来提前预留容器的容量
- 可以移动构造
```c++
std::array<std::string, 5> words {"one", "two", "three", "four", "five"};
std::vector<std::string> words_copy {std::make_move_iterator(std::begin(words)), std::make_move_iterator(std::end(words))};
```
访问元素
- 索引表达式
- front()和back()访问首尾元素
- 调用data()函数来返回数组指针
- 使用迭代器
```c++
vector<double> data;
copy(istream_iterator<double>(cin), istream_iterator<double>(), back_inserter(data));
copy(begin(data), end(data), ostream_iterator<double>(cout, ","));
```
添加元素
- push_back(param)
- emplace_back(param)
- emplace(iterator, param)
- insert(iterator, param);
- insert(iterator, begin_iter, end_iter)
删除元素
- clear()删除所有元素
- pop_back()清除尾部元素
- shrink_to_fit()去掉容器多余的容量
- erase(iterator)去掉迭代器指向的元素
- erase(begin_iter, end_iter)去掉迭代器所指范围的元素
- std::remove(begin_iter, end_iter, param)去掉迭代器范围内匹配param的元素
vector<bool>容器
- 布尔值序列通常不需要存放在连续内存中，因此没有data()函数
- 布尔直接寻址时，会被包装成一个字，因此front()和back()返回值不是bool&引用，而是一个中间对象的引用
- 当使用固定数量的布尔值时, bitset<N>更好，但bitset不是容器

## 2.3 使用deque<T>容器
以双端队列的形式组织元素，可以在容器的头部和尾部高效地添加或删除对象
容器中的元素是序列，但是内部的存储方式与vector不同，导致容器的大小和容量总是相同
访问元素
- 索引表达式
- 使用at()函数
- front()和back()函数
- 元素没有存放在数组中
- 只有size()函数，没有capacity()函数
添加和移除元素
- push_back(), push_front()
- emplace_back(), emplace_front()
- emplace(), insert()
- pop_back(), pop_front(), erase(), clear()
替换deque容器中的内容
- assign({...}), assign(begin_iter, end_iter), assign(num, param)
- 赋值运算符
```c++
std::deque<string> names;
std::copy(std::istream_iterator<string>{std::cin}, std::istream_iterator<string>{}, std::front_inserter(names));
```
## 2.4 使用list<T>容器
定义了T类型对象的双向链表
访问元素
- 双向迭代器，begin(), end(), rbegin(), rend(), cbegin(), cend(), std::begin()...
- front(), back()
- 标准迭代器运算
添加元素
- push_front(), push_back()
- emplace_front(), emplace_back()
- emplace(), insert()
移除元素
- remove(param)
- remove_if( function_obj )
- unique()移除连续的重复元素，一般应该先对元素进行排序后再使用unique
排序和合并元素
- 使用sort()成员函数
- 使用merge()成员函数合并另一个相同类型的容器，这两个容器必须都是升序排列
- 使用splice()移动参数list容器中的元素到当前容器中指定位置的前面
```c++
std::list<std::string> my_words {"six", "three", "nine"};
std::list<std::string> your_words {"six", "seven", "eight"};
my_word.splice(++std::begin(my_words), your_words, ++std::begin(your_words));
```
## 2.5 使用forward_list<T>容器
以单链表的形式存储元素
访问元素
- 前向迭代器, begin(), end(), cbegin(), cend(), std::begin(), std::end()...
- cbefore_begin(), before_begin()
- front(), 没有size()
添加元素
- splice_after()
- insert_after()
移除元素
- remove()
- remove_if()
- unique()
排序和合并元素
- 使用sort()成员函数
- 使用merge()成员函数
