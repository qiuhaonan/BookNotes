# 9.流操作
## 9.1 流迭代器
流迭代器是从流中读取的单通迭代器，它是一个输入流迭代器，或写入流。流迭代器只能传送给定类型的数据到流中或者从流中读取给定类型的数据。如果想用流迭代器来传送一系列不同类型的数据项，就必须将数据项打包到一个单一类型的对象中，并保证这种类型存在流插入和/或流提取运算符。
输入流迭代器
- 可以在文本模式下从流中提取数据的输入迭代器，不能处理二进制流
- 一般用两个流迭代器来从流中读取全部的值，一个指向要读入的第一个值的开始迭代器，一个指向流的末尾的结束迭代器
- 在输入流的文件结束状态被识别时，就可以确定结束迭代器
```c++
std::istream_iterator<string> in {std::cin};
std::istream_iterator<string> end_in; //默认构造函数会生成一个代表流结束的对象
```
istream_iterator对象的成员函数
- operator*()
- operator->()
- operator++()
- operator++(int)
```c++
std::istream_iterator<string> in {std::cin};
std::vector<string> words;
while(true)
{
  string word = *in;
  if(word == "!")
    break;
  words.push_back(word);
  ++in;
}
```
输出流迭代器
ostream_iterator
- 能够将任意T类型对象写到文本模式的输出流中的输出迭代器
- 只要T类型的对象实现了将T类型对象写到流中的operator<<()
- 默认按照char字符的序列来写入值
成员函数
- 构造函数，第一个参数ostream对象的输出流， 第二个参数是分隔字符串
- operator=(const T& obj)会将obj写入到流中，然后写分隔符字符串
- operator*()
- operator++(), operator++(int)
```c++
std::vector<string> words {"The", "quick", "brown"};
std::ostream_iterator<string> out_iter1 {std::cout};

for(const auto& word : words)
{
  *out_iter1++ = word;
  *out_iter1++ = " ";
}
*out_iter1++ = "\n";

for(const auto& word : words)
{
  (out_iter1 = word) = " ";
}
out_iter1 = "\n";

std::ostream_iterator<string> out_iter2 {std::cout, " "};
std::copy(std::begin(words), std::end(words), out_iter2);
out_iter2 = "\n";
```
## 9.2 重载插入和提取运算符
必须为任何想和输出流迭代器一起使用的类类型重载插入和提取运算符
```c++
class Name
{
  private:
    std::string first_name {};
    std::string second_name {};
  public:
    Name() = default;
    Name(const std::string& first, const std::string& second):first_name(first), second_name(second) {}
    std::istream& operator>>(std::istream& in, Name& name)
    {
      return in>>name.first_name>>name.second_name;
    }
    std::ostream& operator<<(std::ostream& out, Name& name)
    {
      return out<<name.first_name<<" "<<name.second_name;
    }
};
```
## 9.3 对文件使用流迭代器
文件流
- 封装了一个实际的文件
- 起始位置是流中索引为0的第一个字符的索引
- 结束位置是文件流中最后一个字符的下一个位置
- 当前位置是下一个写或读操作的开始位置的索引
- 可以用提取和插入运算符来读写数据
文件流类的模板
- ifstream表示文件的输入流
- ofstream表示文件的输出流
- fstream表示文件的读写流
打开状态
- binary会将文件设置成二进制模式
- app在每个写操作之前会移到文件的末尾
- ate会打开文件之后移到文件的末尾
- in
- out
- trunc将当前存在的文件长度截断为0

## 9.4 流缓冲区迭代器
流缓冲区迭代器可以直接访问流的缓冲区
- 不包含插入和提取运算符
- 读写数据时没有数据转换，适用于二进制文件
输入流缓冲区迭代器istreambuf_iterator
- 构造函数，将一个流对象传给构造函数
```c++
std::istreambuf_iterator<char> in {std::cin};
std::istreambuf_iterator<char> end_in;
std::string rubbish {in, end_in};
```
- operator*()返回流中当前字符的副本
- operator->()访问当前字符的成员
- operator++(), operator++(int)
- equal()接受另一个输入流缓冲区迭代器为参数， 当前迭代器和参数都不是流结束迭代器，或者都是流结束迭代器，就会返回true
输出流缓冲区迭代器ostreambuf_iterator
- 构造函数，将一个输出流对象传递给构造函数，或者通过将一个流缓冲区的地址传给构造函数
```c++
std::ofstream junk_out {"/home/qhn/hello.txt"};
std::ostreambuf_iterator<char> out_ {junk_out.rdbuf()};
```
- operator=()会将参数字符写到流缓冲区中
- 当上一次写缓冲区失败时， failed()返回true, 识别到EOF时会报此错误，表明缓冲区满了
- operator*()不做任何事
- operator++(), operator++(int)不做任何事

## 9.5 string流以及流缓冲区迭代器
string流表示内存中字符缓冲区的I/O对象
- basic_istringstream
- basic_ostringstream
- basic_stringstream
这些流对象的属性与文件流相同
