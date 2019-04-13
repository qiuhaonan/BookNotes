# 5. set容器
分类：
- set<T>
- multiset<T>
- unordered_set<T>
- unordered_multiset<T>
集合算法要求：
- 两个迭代器指定的对象集合作为元素段
- 这段元素必须是有序的
- set运算的全部算法需要集合有相同的顺序
- 从任何STL算法创建的对象集合都是原集合对象的副本
集合算法：
- set_union()实现了集合的并集运算
```c++
std::set_union( std::begin(set1), std::end(set1), std::begin(set2), std::end(set2), std::back_inserter(result));
```
- set_intersection()实现了集合的交运算
```c++
std::set_intersection( std::begin(set1), std::end(set1), std::begin(set2), std::end(set2), std::back_inserter(result));
```
- set_difference()实现了集合的差运算
```c++
std::set_difference( std::begin(set1), std::end(set1), std::begin(set2), std::end(set2), std::back_inserter(result));
```
- set_symmetric_difference()实现了集合的对称差运算
```c++
std::set_symmetric_difference( std::begin(set1), std::end(set1), std::begin(set2), std::end(set2), std::back_inserter(result));
```
- includes()算法可以比较两个元素的集合
```c++
std::includes( std::begin(set1), std::end(set1), std::begin(set2), std::end(set2) );
```
