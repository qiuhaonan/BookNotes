# 2018-8-12
## 1.文章病句标识
- Probelm: 为了提高文章质量，每一篇文章都会有m名编辑进行审核，每个编辑独立工作，会把觉得有问题的句子通过下标记录下来，比如[1,10]，1表示病句的第一个字符，10表示病句的最后一个字符。也就是从1到10这10个字符组成的句子，是有问题的。现在需要把多名编辑有问题的句子合并起来，送给总编辑进行最后的审核。比如编辑A支出的病句是[1,10],[32,45]；B编辑指出的病句是[5,16],[78,94]，那么[1,10]和[5,16]是有交叉的，可以合并为[1,16],[32,45],[78,94]。
- Input: 编辑数量m，之后每行是每个编辑的下标集合，每一个和最后一个下标用英文逗号分隔，每组下表之间用分号分隔
- Output: 合并后的下标集合，第一个和最后一个下标用英文逗号分隔，每组下标之间用分号分隔，返回结果是从小到大递增排列
- Sample:
```
Input:
3
1,10;32,45
78,94;5,16
80,100;200,220;16,32
Output:
1,45;78,100;200,220
```
- Range: m < 2^16, 下标数值 < 2^32, 每个编辑记录的下标 < 2^16组
- Analysis:
  - 首先，给定的输入格式比较复杂，既有逗号，又有分号，且尾部没有分隔符，在不知道输入长度的情况下，简单的cin输入无法完成任务。考虑这些分隔符本质上并没有意义，因为记录的格式是固定的，只要每次从输入流中读取两个下标值即可，即便换行也不会造成影响，因此，我们考虑将这些分隔符去除掉，可以降低读取输入的复杂度。
  - 假设现在已经读取完这些记录值了，要对它们进行合并操作，如果不进行排序，那么对于每一个下标，都需要搜索其他所有的下标来判断是否合并，因此，在正式处理之前，应该先对这些记录进行排序。由于合并操作需要考虑一组记录的首尾下标的两端极值，因此我们可以让这些记录按照首下标从小到大进行排序
  - 现在的记录是按照首下标从小到大的顺序排列，如果要合并这些记录，对于第一个记录r，应该向后搜索那些首下标小于r首下标的记录，并筛选出这些记录中最大的尾下标，直到遇到不满足该条件的记录R，那么第一条记录合并的结果就是把尾下标改成最大的尾下标。接着对于r，从记录R开始再次搜索是否有可以合并的记录，如果有，应该继续合并，如果没有，则将R作为第二个准备合并的记录，从R+1向后搜索。
- Solution:
```c++
#include<iostream>
#include<vector>
#include<algorithm>
#include<iterator>
#include<sstream>

using namespace std;

bool compare(const pair<int,int>& a, const pair<int,int>& b)
{
    if(a.first < b.first)
        return true;
    else return false;
}

int main()
{
    int m;
    cin>>m;
    cin.ignore(); //skip '\n'
    vector<pair<int,int>> tag;
    for(int i=0; i<m; ++i)
    {
        int start=0, end=0;
        string line;
        std::getline(std::cin, line); // extract the whole line
        replace(line.begin(), line.end(), ',', ' '); // remove ','
        replace(line.begin(), line.end(), ';', ' '); // remove ';'
        std::istringstream line_stream(line);
        while(line_stream.eof() == false) // whether the line is finished
        {
            line_stream>>start>>end; // read 'start' and 'end' from line
            tag.push_back(std::make_pair(start, end)); // recording
        }
    }
    sort(tag.begin(), tag.end(), compare); // sorting
    for(auto it:tag)
        cout<<"["<<it.first<<","<<it.second<<"] ";
    cout<<endl;
    vector<pair<int,int>> res;
    res.push_back(tag[0]); // processing the first record
    for(int i=1; i<tag.size(); )
    {
        int j=i, max_tail = res.back().second;
        for(; j<tag.size(); ++j) // search records available for merging
        {
            if(tag[j].first > res.back().second)
                break;
            if(tag[j].second > max_tail)
                max_tail = tag[j].second;
        }
        res.back().second = max_tail;
        if(j == i) // no results
        {
            i = i+1;
            res.push_back(tag[j]); // forward iteration 
        }
        else i = j;
    }
    for(auto it:res)
        cout<<"["<<it.first<<","<<it.second<<"] ";
    cout<<endl;

    return 0;
}
```

# 2. 区间最大最小值
- Problem: 两个长度为n的序列a，b，问有多少个区间[l,r]满足 max(a[l,r]) < min(b[l,r])
- Input： 第一行一个整数n， 第二行n个数， 第i个为ai， 第三行n个数， 第i个为bi
- Output： 一行一个整数
- Sample：
```
Input
3
3 2 1
3 3 3
Output
3
```
- Range： n < 1e5, ai,bi < 1e9
- Analysis:
  - 假如已经知道两个序列任意区间的最大值和最小值，那么判断有多少个区间满足条件，双重循环就能够解决问题。
  - 怎么才能知道任意区间的最大值和最小值呢，暴力法就是使用标准库算法的std::max_element(begin, end)和std::min_element(begin,end)函数，由于这两个函数都是暴力搜索，搭配双重循环，时间复杂度将是O(N^3)，因此，需要将极值搜索的时间复杂度在循环中降低
  - 考虑提前计算极值，在循环判断中只需访问结果即可
```c++
#include<iostream>
#include<vector>
#include<numeric>
#include<algorithm>
#include<random>
using namespace std;

void push_max(vector<int>& arr, vector<int>& max_arr, int start)
{
    int max_val = arr[start];
    for(int i = start; i < arr.size(); ++i)
    {
        if(arr[i] > max_val)
            max_val = arr[i];
        max_arr[i] = max_val;
    }
}

void push_min(vector<int>& arr, vector<int>& min_arr, int start)
{
    int min_val = arr[start];
    for(int i = start; i < arr.size(); ++i)
    {
        if(arr[i] < min_val)
            min_val = arr[i];
        min_arr[i] = min_val;
    }
}

void print(vector<vector<int>>& arr, string title)
{
    cout<<title<<endl;
    for(auto row:arr)
    {
        for(auto col:row)
            cout<<col<<" ";
        cout<<endl;
    }
}

int main()
{
    int n = 10;
    std::random_device rd;
    std::default_random_engine rng(rd()); 
    vector<int> a(n), b(n);
    for(int i=0; i<n; ++i)
    {
        a[i] = rd() % 20;
        b[i] = rd() % 20;
    }
    for(auto i:a)
        cout<<i<<", ";
    cout<<endl;
    for(auto i:b)
        cout<<i<<", ";
    cout<<endl;
    vector<vector<int>> a_max, b_min;
    for(int i=0; i<n; ++i)
    {
        vector<int> max_arr(n, -1);
        push_max(a, max_arr, i);
        a_max.push_back(max_arr);
        vector<int> min_arr(n, -1);
        push_min(b, min_arr, i);
        b_min.push_back(min_arr);
    }
    
    print(a_max, "max");
    print(b_min, "min");

    int num = 0;
    for(int i=0; i<n; ++i)
    {
        for(int j=i; j<n; ++j)
        {
            if(a_max[i][j] < b_min[i][j])
            {
                ++num;
                cout<<"["<<i<<","<<j<<"] ";
            }
        }
        cout<<endl;
    }
    cout<<num<<endl;
}
```
