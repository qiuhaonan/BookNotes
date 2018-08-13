# 6. 排序、合并、搜索和分区
## 6.1 序列排序
排序
- sort()默认会将元素段排成升序
- 排序的对象的类型需要支持<运算符
- 对象必须可交换，swap()
- 随机访问迭代器，只能是array, vector, deque, 或标准数组
相等元素的排序
- stable_sort()可以保证维持相等元素的原始顺序
部分排序
- partial_sort()
```c++
size_t count {5};
std::vector<int> numbers {22, 7, 93, 45, 19, 56, 88, 12, 8, 7, 15, 10};
std::partial_sort( std::begin(numbers), std::end(numbers) + count, std::end(numbers));
```
- nth_element()
```c++
std::nth_element( std::begin(numbers), std::end(numbers) + count, std::end(numbers));
```
- is_sorted() 和 is_sorted_until()判断一个元素段是否有序
```c++
std::is_sorted( std::begin(numbers), std::end(numbers));
std::is_sorted_until( std::begin(numbers), std::end(numbers));
```
合并序列
- 合并两个有相同顺序的序列中的元素， 结果会产生一个包含来自这两个输入序列的元素的副本的序列，并且排列方式和原始序列相同
- merge()
```c++
std::merge(std::begin(these), std::end(these), std::begin(those), std::end(those), std::begin(result))
```
- inplace_merge()可以合并同一个序列中两个连续有序的元素序列
## 6.2 搜索序列
在序列中查找元素
- find(), find_if(), find_if_not()
在序列中查找任意范围的元素
- find_first_of()
在序列中查找多个元素
- adjacent_find()可以搜索序列中两个连续相等的元素
- find_end()在一个序列中查找最后一个和另一个元素段匹配的匹配项
- search()在一个序列中查找第一个匹配项
- search_n()会搜索给定元素的匹配项，它在序列中连续出现了给定的次数
## 6.3 分区序列
在序列中分区元素会重新对元素进行排列，所有使给定谓词返回true的元素会被放在所有使谓词返回false的元素前面
- partition
```c++
std::partition( std::begin(numbers), std::end(numbers), [](int a) {return a > 100;})
```
- stable_partition()
- partition_copy()使结果复制到不同的序列中，而不会改变原始序列
```c++
std::partition_copy( std::begin(numbers), std::end(numbers), std::back_inserter(res1), std::back_inserter(res2), [](int a) {return a > 100;})
```
- partition_point()获取分区序列中第一个分区的结束迭代器
- is_partitioned()用来判断序列是否已经分区
## 6.4 二分查找算法
对于有序序列，二分查找效率比顺序搜索更高
- binary_search()
```c++
auto find_bool = std::binary_search( std::begin(values), std::end(values), wanted);
```
- lower_bound()可以在前两个参数指定的范围内查找不小于第三个参数的第一个元素
```c++
std::lower_bound( std::begin(values), std::end(values), wanted );
```
- equal_range()可以找出有序序列中所有和给定元素相等的元素
```c++
auto pr = std::equal_range( std::begin(values), std::end(values), wanted );
```
