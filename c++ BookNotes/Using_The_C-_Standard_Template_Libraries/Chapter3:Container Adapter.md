# 3. 容器适配器
容器适配器是一个封装了序列容器的类模板，它在一般序列容器的基础上提供了一些不同的功能。
## 3.1 使用stack<T>容器适配器
stack<T>容器适配器中的数据是以LIFO的方式组织的
创建容器
- 构造容器适配器的模板有两个参数，第一个参数是存储对象的类型，第二个参数是底层容器的类型，默认是deque<T>
- 使用另一个容器来初始化，只要底层容器类型相同即可
- 不能使用初始化列表
堆栈操作
- top()
- push()
- pop()
- emplace()
- size()
- empty()
- swap()
赋值和比较操作
- 复制和移动的operator=()函数
- 比较运算通过字典的方式来比较底层容器中相应的元素，其次是按元素数量比较
遍历操作
- 遍历容器内容，并移除访问过的每一个元素
- 没有迭代器
## 3.2 使用queue<T>容器适配器
以FIFO的准则处理序列
只能访问容器的第一个和最后一个元素；另外只能在容器的末尾添加新元素，在头部移除元素；
创建容器
- 构造容器适配器的模板有两个参数，第一个参数是存储对象的类型，第二个参数是底层容器的类型，默认是deque<T>
- 使用另一个容器来初始化，只要底层容器类型相同即可
- 不能使用初始化列表
队列操作
- front()
- back()
- push()
- pop()
- size()
- empty()
- emplace()
- swap()
赋值和比较操作
- 复制和移动的operator=()函数
- 比较运算通过字典的方式来比较底层容器中相应的元素，其次是按元素数量比较
遍历操作
- 遍历容器内容，并移除访问过的每一个元素
- 没有迭代器
## 3.3 使用priority_queue<T>容器适配器
priority_queue<T>容器适配器定义了一个元素有序排列的队列
默认队列头部的元素优先级最高，只能访问第一个元素
创建容器
- 构造函数有三个参数，第一个参数是存储对象的类型，第二个参数是存储元素的底层容器(默认是vector<T>)，第三个参数是函数对象
```c++
template<typename T, typename Cotainer=std::vector<T>, typename Compare=std::less<T>>
class priority_queue
```
- 底层容器类型可以是任何容器
优先级队列操作
- push()
- emplace()
- top()
- pop()
- size()
- empty()
- swap()
遍历操作
- 没有迭代器
- 列出或复制，会将队列清空
## 3.5 堆
堆不是容器， 而是一种特别的数据组织方式， 一般用来保存序列容器。
创建堆
- make_heap()对随机访问迭代器指定的一段元素重新排列，默认使用<运算符，生成一个大顶堆
堆操作
- push_heap()认为最后一个元素是新元素，然后重新排列序列
- pop_heap()将第一个元素移到最后，并保证剩下的元素仍然是一个堆，此后可以使用底层容器pop_back()彻底删除最后一个元素
- is_heap()可以检查序列是否仍然是堆
- is_heap_until()返回一个迭代器，指向第一个不在堆内的元素
- sort_heap()将元素段作为堆来排序如果元素段不是堆，程序会崩溃，排序操作的结果是有序的，虽然堆并不是全部有序的，但是任何全部有序的序列都是堆
## 3.6 在容器中保存指针
通常用容器保存指针比保存对象更好，而且大多时候保存智能指针比原生指针好
```c++
using PString = std::shared_ptr<std::string>
std::vector<PString> words;
```
在优先级队列中保存指针
- 需要提供一个自定义比较函数，比较两个智能指针所指向的对象
在堆中保存指针
- 需要提供一个用来比较指针的函数指针
对指针序列应用算法
- 提供一个函数对象来自定义运算，例如
```c++
using word_ptr = std::shared_ptr<string>;
std::vector<word_ptr> words;
words.push_back(std::make_shared<string>("one"));
words.push_back(std::make_shared<string>("two"));
words.push_back(std::make_shared<string>("three"));
auto str = std::accumulate(std::begin(words), std::end(words), string(""), [](const string& s, const word_ptr& pw) -> string { return s + *pw + " "; });
```
