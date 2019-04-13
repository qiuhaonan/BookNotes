# 8. 生成随机数
使用STL生成随机数：
- 随机数生成引擎
  - 一个定义了生成随机位序列的无符号整数序列机制的类模板
- 随机数生成器
  - 随机数引擎类模板的一个预定义的实例
- 随机数引擎适配器
  - 一个类模板，它通过修改另一个随机数引擎生成的序列来生成随机数序列
- 分布
  - 随机序列中的数出现在序列中的概率
## 8.1 随机种子
获取随机种子
- random_device类定义的函数对象，可以生成用来作为种子的随机的无符号整数值
种子序列
- seed_seq类是用来帮助设置随机数生成器的初始状态的
## 8.2 分布类
STL中的分布是一个函数对象，一个表示特定分布的类的实例
- 应该总是使用分布对象来获取随机数
默认随机数生成器
- std::default_random_engine
```c++
std::random_device rd;
std::seed_seq sd_seq {2,4,6,8};
std::default_random_engine rng {rd()};
std::default_random_engine rng {sd_seq};
```
创建分布对象
- 为了产生分布内的值， 表示分布的函数对象需要一个随机数生成器对象作为参数
- 每次执行分布对象， 都会返回一个在它所表示的分布之内的随机数， 这个随机数是从随机数生成器所获得的值生成的
均匀分布
- 离散均匀分布
  - std::uniform_int_distribution返回均匀分布在闭合范围[a,b]内的随机整数
- 连续均匀分布
  - std::uniform_real_distribution返回均匀分布在[a,b)范围内的随机double型浮点值
- 标准均匀分布
  - std::generate_canonical()提供一个浮点值范围在[0,1)内，且有给定的随机比特数的标准均匀分布
  - 第一个参数是浮点类型， 第二个参数是尾数的随机比特数的个数， 第三个参数是使用的随机数生成器的类型
正态分布
- std::uniform_distribution定义了可以产生随机浮点数值的分布对象类型，默认是double类型，默认构造函数创建标准正态分布
- std::chi_squared_distribution
- std::cauchy_distribution
- std::fisher_f_distribution
- student_t_distribution
对数分布
- std::lognormal_distribution定义了默认返回浮点类型值的对数分布对象
抽样分布
- 离散分布
  - std::discrete_distribution定义了返回随机整数的范围在[0,n)内的分布， 基于每个从0到n-1的可能值的概率权重
- 分段常数分布
  - std::piecewise_constant_distribution定义了一个在一组分段子区间生成浮点值的分布，给定子区间内的值是均匀分布的，每个子区间都有自己的权重
- 分段线性分布
  - std::piecewise_linear_distribution定义了浮点值的连续分布，它的概率密度函数是从一系列的样本值所定义的点得到的
其他分布
- 泊松分布
  - std::poisson_distribution
- 几何分布
  - std::geometric_distribution
- 指数分布
  - std::exponential_distribution
- 伽马分布
  - std::gamma_distribution
- 威布尔分布
  - std::weibull_distribution
- 二项式分布
  - std::binomial_distribution
- 负二项式分布
  - std::negative_binomial_distribution
- 极值分布
  - std::extreme_value_distribution
## 8.3 随机数生成引擎和生成器
随机数引擎
- 线性同余引擎， std::linear_congruential_engine
- 马特赛特旋转演算法引擎, std::mersenne_twister_engine
- 带进位减法引擎, std::subtract_with_carry_engine
随机数引擎适配器
- idependent_bits_engine将引擎生成的值修改为指定的比特个数
- discard_block_engine将引擎生成的值修改为丢弃给定长度的值序列中的一些元素
- shuffle_order_engine将引擎生成的值返回到不同的序列中
随机数生成器
- minstd_rand0, minstd_rand, knuth_b
- mt19937, mt19937_64
- ranlux24_base, ranlux48_base, ranlux24, ranlux48
- default_random_engine
