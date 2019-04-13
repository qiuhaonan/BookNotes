# 4. map容器
map容器是关联容器的一种，在关联容器中，对象的位置取决于和它关联的键的值。
map容器的元素是pair<const  K, T>类型的对象，键为const是因为修改键会扰乱容器中元素的顺序。
map容器分类：
- map<K,T>
  - 默认使用less<K>对象比较
- multimap<K,T>
  - 允许使用重复的键
- unordered_map<K,T>
  - 元素的顺序是由键值的哈希值决定的
- unordered_multimap<K,T>
  - 允许使用重复的键
虽然map容器会使用less<K>对元素进行排序，但是这些元素并没有被组织成一个简单的有序序列，而是一般被保存在一个平衡二叉树中
## 4.1 map
创建容器
- 在构造函数中指定K和T的类型
- 使用嵌套初始化列表来初始化map容器
```c++
std::map<std::string, size_t> people { {"Ann", 25}, {"Bill", 45}, {"Jack", 32}};
```
- 使用初始化列表加上pair对象来初始化map容器
```c++
std::map<std::string, size_t> people { std::make_pair("Ann", 25), std::make_pair("Bill", 45), std::make_pair("Jack", 32)};
```
- 移动和复制构造函数
- 另一个容器的一段元素的迭代器来初始化一个map容器
添加元素
- insert()
- emplace()
- emplace_hint()
访问元素
- at()
- 迭代器
- find()
- equal_range()
- upper_bound()
- lower_bound()
删除元素
- erase()
- clear()
## 4.2 pair和tuple
pair<K,T>的操作
- 成员变量first, second
- 使用tuple参数来构造pair
- make_pair()
```c++
std::pair<Name, Name> couple { std::piecewise_construct, std::forward_as_tuple("Jack", "Jones"), std::forward_as_tuple("Jill", "Smith") };
```
- 赋值运算符，比较运算符(按照字典序比较)
tuple<...>的操作
- make_tuple
- 使用pair参数来构造tuple
- 比较运算符(按照字典序比较)
- 使用std::get<n>()来访问tuple中的元素
- 使用std::tie<>()把tuple中的元素值转换为可以绑定到tie<>()的左值集合
```c++
auto my_tuple = std::make_tuple(Name{"Peter", "Piper"}, 42, std::string{"914 626 7890"});
Name name{};
size_t age{};
std::string phone{};
std::tie(name, age, phone) = my_tuple;
std::tie(name, std::ignore, phone) = my_tuple;
```
- 可以使用tie函数来实现对类的数据成员的字典比较
```c++
bool Name::operator<(const Name& name) const
{
  return std::tie(second, first) < std::tie(name.second, name.first);
}
```
## 4.3 unordered_map容器
unordered_map包含的是有唯一键的键/值对元素。容器中的元素不是有序的。元素的位置由键的哈希值确定，因而必须有一个适用于键类型的哈希函数。如果用类对象作为键，需要为它定义一个实现了哈希函数的函数对象。如果键是STL提供的类型，通过特例化hash<T>，容器可以生成这种键对应的哈希函数。
生成和管理容器
- 初始化列表
- 舒适化列表和需要分配的格子数
- 迭代器对和需要分配的格子数
- 自定义函数对象
```c++
class Name
{
  private:
    std::string first{};
    std::string second{};
  public:
    size_t hash() const
    {
      return std::hash<std::string>()(first+second);
    }
    bool operator==(const Name& name) const
    {
      return first == name.first && second == name.second;
    }
};
class Hash_Name
{
  public:
    size_t operator()(const Name& name) const 
    {
      return name.hash();
    }
};
std::unordered_map<Name, size_t, Hash_Name> people {{{"Ann", "Ounce"}, 25}, {{"Bill", "Bao"}, 46}, 500, Hash_Name()};
```
- 调用rehash()来调整格子个数
- 调用max_load_factor()改变最大负载因子来调整格子数
- 调用reserve()直接设置格子个数
插入元素
- insert()
- emplace()
- emplace_hint()
- 赋值运算符
访问元素
- 索引表达式, 当下标使用不存在的键时，会以这个键为新键生成一个新的元素，新元素中对象的值是默认的
- at()返回参数所关联对象的引用
- find(), equal_range()
- 迭代器， for循环
移除元素
- erase()
- clear()
