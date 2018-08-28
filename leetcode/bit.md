# [Leetcode] Bit Manipulation
# 1. 概念
位操作是从算法上操纵比一个字长更短的数据片或者数据位的行为。需要位操作的任务包括底层设备控制，错误检测和修正算法，数据压缩，加密算法和优化。现代编程语言允许编程人员通过AND，OR，XOR，NOT和移位操作直接操纵bit位。

在某些情况下，位操作可以减少在一个数据结构上循环的必要，并且由于位操作可以并行处理，能获得很高的数据加速，但是代码可能会变得难以编写和维护。

# 2. 基本功能
bit_fld是一个无符号字符变量，对其中第n位(从0开始)执行特定操作：
- 置位
    - bit_fld |= (1 << n)
- 清零
    - bit_fld &= ~(1 << n)
- 切换
    - bit_fld ^= (1 << n)
- 测试
    - bit_fld & (1 << n)

例如：判断一个无符号数是否是2的幂
```c
bool isPowerOfTwo = x && !(x & (x-1));
```

# 3. 一些神奇的功能
## 3.1 float类型的绝对值
IEEE 754浮点值的最高位是符号位:1代表负值，0代表正数包括0，因此，只要把这一符号位设成0即可。float类型要求32位，但是c/c++都不提供对float的位操作。为了解决问题，可以把这32位解释成一个32位整数，然后清除符号位，返回时再解释成float类型。
```c
float myAbs(float x)
{
    int casted = *(int*) &x;
    casted &= 0x7FFFFFFF;
    return *(float*)&casted;
}
```

## 3.2 int类型的绝对值
所有主流处理器上都使用二进制补码来表达负整数：对于x>=0, 就是x+0，对于x<0, 就是NOT(x)+1(符号位以后取反加1)。而在底层，计算机提供逻辑移位和算数移位。区别在于右移时，逻辑移位在高位补零，而算数移位在高位重复之前的最高位。在c/c++中，有符号整数是算数移位，无符号整数是逻辑移位。怎么在补码计算中根据数据的正负得到0和1呢？换个方式，0还是0,1其实是-(-1)，也就是NOT(x)-(-1)，进而就是得到0和-1。也就是说当x大于0时，应该能得到0，而x小于0时得到-1。又因为x是正数时，移位操作是逻辑移位，而负数是算数移位，那么把x右移31位，对于正数(最大2^31 - 1)来说结果就是0x00000000(0)，而对于负数(最小-2^31)就是0xFFFFFFFF(-1)。另外，由于任意数和0异或是本身，和1异或是取反，正好与补码计算规则一致。
```c
int myAbs(int x)
{
    const int bit31 = x >> 31;
    return (x ^ bit31) - bit31;
}
```

## 3.3 统计一个32位无符号整数中1的数量
先每2位统计一次，再每4位统计一次，接着每8位统计一次，最后求和
```c
unsigned int countBits(unsigned int x)
{
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = x + (x >> 4);
    x &= 0xF0F0F0F;
    return (x * 0x01010101) >> 24;
}
```

## 3.4 检测无符号32位整数中是否有0字节存在
如果某个字节为0，那么16进制表示就是0x00，0x00的特性是减去1变成0xFF，或者直接取反变成0xFF，而其他数值均没有此特性，因此，通过这两种操作的结果相与，可以测试每个字节是否是0，如果是0，那么该字节相与不会发生变化，否则字节结果是0，最后提取出每个字节中的最高位，判断结果是否是0，如果是，那么不包含0字节，否则就有0字节存在
```c
bool hasZeroByte(unsigned int x)
{
    return ((x - 0x01010101) & ~x & 0x80808080) != 0;
}
```

## 3.5 判断大端还是小端
小端模式，是指数据的高字节保存在内存的高地址中，而数据的低字节保存在内存的低地址中，这种存储模式将地址的高低和数据位权有效地结合起来，高地址部分权值高，低地址部分权值低。
```c
bool isLittleEndian()
{
    short pattern = 0x0001;
    return *(char*)&pattern == 0x01;
}
```

## 3.6 扩展位宽
对于图形卡，值通常被截断为几个比特，即每个颜色通道5比特。当用于实际绘制像素时，这些值必须扩展到8位，同时保留三个重要属性：
- 缩放必须尽可能线性
- 零必须保持为零
- 必须将最大可能输入映射到最大可能输出
简单的左移3位能够满足前两个条件，但是第三个条件不满足，例如0x1F变成0xF8，而不是0xFF，因此需要通过右移3位来把最高3位映射到最低3位

```c
unsigned int extend(unsigned int value, unsigned int smallSize, unsigned int bigSize)
{
    unsigned int leftShift = bigSize - smallSize;
    unsigned int rightShift = bigSize - leftShift;
    return (value << leftShift) | (value >> rightShift);
}
```

## 3.7 判断一个32位无符号整数中1的奇偶属性
由于一个整体的奇偶位属性可以由多个子块的奇偶位属性共同决定，那么就可以把32位整数打散成多个4位的块。异或运算可以在两个块之间决定奇偶位属性，即一个块是偶属性，另一个是奇属性，那么结果就是奇属性，与异或运算规则相符。经过三次“折叠”后，低4位保存了整个32位的奇偶信息，因此通过0x0F相与可以保留该信息。而剩下的4位只有16种可能的值：\
parity(15) → parity(1111b) → 4 bits set → even\
parity(14) → parity(1110b) → 3 bits set → odd\
parity(13) → parity(1101b) → 3 bits set → odd\
parity(12) → parity(1100b) → 2 bits set → even\
parity(11) → parity(1011b) → 3 bits set → odd\
parity(10) → parity(1010b) → 2 bits set → even\
parity( 9) → parity(1001b) → 2 bits set → even\
parity( 8) → parity(1000b) → 1 bits set → odd\
parity( 7) → parity(0111b) → 3 bits set → odd\
parity( 6) → parity(0110b) → 2 bits set → even\
parity( 5) → parity(0101b) → 2 bits set → even\
parity( 4) → parity(0100b) → 1 bits set → odd\
parity( 3) → parity(0011b) → 2 bits set → even\
parity( 2) → parity(0010b) → 1 bits set → odd\
parity( 1) → parity(0001b) → 1 bits set → odd\
parity( 0) → parity(0000b) → 0 bits set → even\
把这16种可能的值从高到低编码成二进制位，结果就是0110 1001 0110 1001，即0x6996，x的值就相当于0x6996中的索引，因此，通过对0x6996右移x的值，把奇偶属性放置到最低位，在与1相与保留最低位，判断是否是0即可。
```c
bool parity(unsigned int x)
{
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x &= 0x0F;
    return ((0x6996 >> x) & 1) != 0;
}
```

## 3.8 查找32位无符号整数中最低位1的位置
负数通过二进制的补码形式来表示，也就是 -x = not(x) + 1。公式 x & -x = x & (not(x) + 1)可以删除除了最低位被设为1的以外其他所有的1。那么值就是2的幂且只有32种可能性，通过和DeBruijn常量相乘，能把每个2的幂高5位设置成为不同的，那么通过查表就可以得到1的位置。
```c
const unsigned int MultiplyDeBruijnBitPosition[32] =
{
   // precomputed lookup table
   0,  1, 28,  2, 29, 14, 24,  3, 30, 22, 20, 15, 25, 17,  4,  8,
   31, 27, 13, 23, 21, 19, 16,  7, 26, 12, 18,  6, 11,  5, 10,  9
};
 
unsigned int lowestBitSet(unsigned int x)
{
   // only lowest bit will remain 1, all others become 0
   x  &= -int(x);
   // DeBruijn constant
   x  *= 0x077CB531;
   // the upper 5 bits are unique, skip the rest (32 - 5 = 27)
   x >>= 27;
   // convert to actual position
   return MultiplyDeBruijnBitPosition[x];
}
```

## 3.9 把32位无符号整数转换成下一个2的幂
只要不是0，那么每个数至少有一位被置为1.考虑只有有效最高位被置为1，那么把有效最高位的1传播到所有的低位，然后再加1就可以得到一个2的幂了。
```c
unsigned int roundUpToNextPowerOfTwo(unsigned int x)
{
    x--;//如果x本身就是2的幂呢
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
}
```

## 3.10 判断两个整数是否是同符号
因为带符号整数通过二进制补码来表示，最高位表示符号位，0代表0和正数的符号，1代表负数，通过异或运算，如果两个数的符号相同，结果就是0，否则结果就是1
```c
bool sameSign(int x, int y)
{
    return (x ^ y) >= 0;    
}
```

## 3.11 判断某个数的符号
通过对带符号整数的右移31位，可以把零正数变成0，把负数变成-1。通过对整数符号取反，再右移31位，可以把零变成0，把正数变成1，把负数变成0。这两个结果组合起来就可以判断三种符号。
```c
int sign(int x)
{
    int minusOne = x >> 31;
    unsigned int negateX = (unsigned int) -x;
    int plusOne = (int)(negateX >> 31);
    int result = minusOne | plusOne;
    return result;
}
```

## 3.12 交换两个数的值
基于a XOR b XOR a可以还原出b，因为自身和自身异或结果是0，而0和任何数值异或结果是数值本身
```c
void swapXor(int& a, int& b)
{
    a ^= b;
    b ^= a;
    a ^= b;
}
```

# 4. 题目
## 4.1 Single Number
- Problem: 给定一个非空整数数组，只有一个元素出现一次，其他元素都出现两次，找到这个元素
- Input: 数组
- Output: 只出现一次的元素
- Sample: [2,2,1], return 1
- Analysis: 由于每个数与自身异或操作会归零，因此可以用异或操作来过滤出现偶数次的元素
- Solution:
```c
class Solution {
public:
    int singleNumber(vector<int>& nums) {
        int res = 0;
        for(auto iter:nums)
            res^=iter;
        return res;
    }
};
```
## 4.2 Majority Element
- Problem: 给定一个数组，找到主要元素，主要元素的出现次数大于等于数组长度的一半，假定数组非空，且主要元素总是存在。
- Input: 数组
- Output: 主要元素
- Sample: [3,2,3], return 3
- Analysis: 主要元素的特征是出现次数大于数组长度的一半，那么也就意味这元素内部的每个bit的出现次数也是大于数组长度的一半。如果主要元素的bit与其他元素的bit重复了，不会影响结果，否则，主要元素的bit出现次数一定大于其他所有元素的bit出现次数，因此，也不会影响结果。
- Solution:
```c
class Solution {
public:
    int majorityElement(vector<int>& nums) {
        int major = 0, n = nums.size();
        for (int i = 0, mask = 1; i < 32; i++, mask <<= 1) {
            int bitCounts = 0;
            for (int j = 0; j < n; j++) {
                if (nums[j] & mask) bitCounts++;
                if (bitCounts > n / 2) {
                    major |= mask;
                    break;
                }
            }
        } 
        return major;
    } 
};

Moore Vote Algorithm
class Solution {
public:
    int majorityElement(vector<int>& nums) {
        int major, counts = 0, n = nums.size();
        for (int i = 0; i < n; i++) {
            if (!counts) {
                major = nums[i];
                counts = 1;
            }
            else counts += (nums[i] == major) ? 1 : -1;
        }
        return major;
    }
};
```
## 4.3 Reverse Bits
- Problem: 将一个32位无符号整数按位逆序
- Input: 一个32位无符号整数
- Output: 按位逆序后的32位无符号整数
- Sample: 43261596， return 964176192
- Analysis: 从最低位开始提取bit，存放到结果的最高位即可。涉及到测试位和置位以及移位操作。
- Solution:
```c
class Solution {
public:
    uint32_t reverseBits(uint32_t n) {
        int result = 0;
        for (int i = 0;i<32;i++){
            int end = n & 1;
            n >>= 1;
            result <<=1;
            result |= end;
        }
        return result;
    }
};
```
## 4.4 Missing Number
- Problem: 给定一个数组包含n个不同的数，取自[0,1,...,n],找到丢失的那个数
- Input: 一个数组
- Output: 丢失的数
- Sample: [3,0,1], return 2
- Analysis: 由于数字取自[0,...,n]并且索引也是[0,...,n]，而数字缺了一个数，那么把索引和数字放在一起看，就出现了丢失的数字只出现了一次，而其他数字出现了两次。异或运算可以找出这个丢失的数字。
- Solution:
```c
class Solution {
public:
    int missingNumber(vector<int>& nums) {
        int missing =0;
        for(int i=0; i<nums.size();++i) 
            missing ^= ((i+1)^nums[i]);
        return missing;
    }
};
```
## 4.5 Power of Four
- Problem: 给定一个有符号的整数，判断是否是4的幂
- Input: 一个有符号整数
- Output: true or false
- Sample: 16, true
- Analysis: 判断是否是4的幂，可以先利用判断2的幂的方法确定只有一个1存在，然后判断1出现在奇数位还是在偶数位来区分2的幂和4的幂，或者通过模3来判断也可以。
- Solution:
```c
public class Solution {
    public boolean isPowerOfFour(int num) {
        return (num > 0) && ((num & (num - 1)) == 0) && ((num & 0x55555555) == num);
    }
}

class Solution {
public:
    bool isPowerOfFour(int num) {
        return (num > 0) && ((num-1)&num) ==0 && (num-1)%3==0;
    }
};
```
## 4.6 Sum of Two Integers
- Problem: 计算连个整数的和，但不准使用+和-操作符
- Input: 两个整数
- Output: 和
- Sample: 1,1 return 2
- Analysis: 在bit上求和基本思路是提取每一位，然后异或求和，通过相与求进位
- Solution:
```c
class Solution {
public:
    int getSum(int a, int b) {
        int sum = a;
        while (b != 0)
        {
            sum = a ^ b;//calculate sum of a and b without thinking the carry 
            b = (a & b) << 1;//calculate the carry
            a = sum;//add sum(without carry) and carry
        }
        return sum;
    }
};
```
## 4.7 Find the Difference
- Problem: 给定两个只包含小写字母的字符串，一个字符串是通过打乱另一个字符串的顺序并添加一个字符得到的，找到添加的这个字符。
- Input: 两个字符串
- Output: 添加的字符
- Sample: "abcd", “abcde”， return 'e'
- Analysis: 由于两个字符串除了添加的字符外，其他字符出现的频率一样，因此可以考虑将连个字符串放在一起看做一个集合，通过异或运算找出只出现一次的字符。
- Solution:
```c
class Solution {
public:
    char findTheDifference(string s, string t) {
        char r=0;
        for(char c:s) r ^=c;
        for(char c:t) r ^=c;
        return r;
    }
};
```
## 4.8 Binary Watch
- Problem: 一个二进制手表，有4个led灯代表小时，6个led灯代表分钟。每个led登要么是0，要么是1，最低位在最右边。给定n个可亮led灯，请给出所有可能代表的时间
- Input: n个可亮led灯
- Output: 所有可能代表的时间, 小时之前没有前导0，分钟必须有前导0
- Sample: n=1, return ["1:00", "2:00", "4:00", "8:00", "0:01", "0:02", "0:04", "0:08", "0:16", "0:32"].
- Analysis: 题目要求找到所有的可能结果，因此可以使用回溯法来搜索，但是本题情况不同，对于每一个要搜索的值的范围是已知的，比如小时是[0,12)，分钟是[0,60)，因此，可以反过来求解，通过对所有可能解进行遍历，判断是否是可行解，可行解满足小时和分钟组成的数中1的位数必须等于可用的led数目。
- Solution:
```c
class Solution {
public:
    vector<string> readBinaryWatch(int num) {
        vector<string> rs;
        for (int h = 0; h < 12; h++)
            for (int m = 0; m < 60; m++)
                if (bitset<10>(h << 6 | m).count() == num)
                    rs.emplace_back(to_string(h) + (m < 10 ? ":0" : ":") + to_string(m));
        return rs;
    }
};
```
## 4.9 Convert a Number to Hexadecimal
- Problem: 给定一个整数，转换成16进制数。这里负数使用二进制补码表示。
- Input: 一个整数
- Output: 16进制数， 不能有前导0，
- Sample: 26， “1a”
- Analysis: 由于可以从位操作中观察给定的数的二进制表达形式，那么可以提取出每4位的值，转换成16进制即可，这里16进制可以直接通过自定义的数组来表达，而不去要在ascii码表中计算。
- Solution:
```c
const string HEX = "0123456789abcdef";
class Solution {
public:
    string toHex(int num) {
        if (num == 0) return "0";
        string result;
        int count = 0;
        while (num && count++ < 8) {
            result = HEX[(num & 0xf)] + result;
            num >>= 4;
        }
        return result;
    }
};
```
## 4.10 Hamming Distance
- Problem: 两个整数间的汉明距是对应bit不同的数量，给定两个整数，计算汉明距。
- Input: 两个整数
- Output: 汉明距
- Sample: 1,4 return 2
- Analysis: 由于汉明距是在统计不同bit的数量，因此，可以通过异或运算来求出两个整数每个bit上是否不同，最后统计1的数量即可。
- Solution:
```c
class Solution {
public:
    int hammingDistance(int x, int y) {
        int dist = 0, n = x ^ y;
        while (n) {
            ++dist;
            n &= n - 1;
        }
        return dist;
    }
};
```
## 4.11 Number Complement
- Problem: 给定一个正整数，输出它的补数，补数是对每一个有效二进制位进行反转操作。
- Input: 一个正整数， 二进制表达式前面没有0。
- Output: 补数
- Sample: 5， return 2
- Analysis: 找到num的最高有效位，然后对num取反，再去掉最高有效位之前的数
- Solution:
```c
class Solution {
public:
    int findComplement(int num) {
        unsigned mask = ~0;
        while (num & mask) mask <<= 1;
        return ~mask & ~num;
    }
};
```
## 4.12 Binary Number with Alternating Bits
- Problem: 给定一个正整数，检查有效位范围内是否相邻位不同
- Input: 一个正整数
- Output: true or false
- Sample: 5, return true
- Analysis: 从最低位开始向高位邻近位判断，当判断失败时，检查剩余的值是否为0，如果是就应该返回true.
- Solution:
```c
class Solution {
public:
    bool hasAlternatingBits(int n) {
        int d = n&1;
        while ((n&1) == d) {
            d = 1-d;
            n >>= 1;
        }
        return n == 0;
    }
};
```
## 4.13 Subsets
- Problem: 给定包含不同整数的集合，返回该集合所有可能的子集
- Input: 集合
- Output: 所有子集
- Sample: [1,2,3], return [[],[1],[2],[3],[1,2],[1,3],[2,3],[1,2,3]]
- Analysis: 由于每个元素在子集中只有两种情况：出现/不出现，因此，对于第一个元素会出现在两个连续的子集中，而第二个元素则会在第一个元素子集基础上出现，因此就会在四个连续的子集中出现，以此类推，第三个元素就在八个连续的子集中出现...现在只要计算出每个元素会出现在哪些子集中即可，因为出现的子集数正好是2的幂，因此可以使用位操作来计算。
- Solution:
```c
class Solution {
public:
    vector<vector<int>> subsets(vector<int>& nums) {
        int n = pow(2, nums.size()); 
        vector<vector<int>> subs(n, vector<int>());
        for (int i = 0; i < nums.size(); i++) {
            for (int j = 0; j < n; j++) {
                if ((j >> i) & 1) {
                    subs[j].push_back(nums[i]);
                }
            }
        }
        return subs;
    }
};
```
## 4.14 Single Number III
- Problem: 给定一个数组，其中只有两个元素出现了一次，其他元素都出现两次，找到这两个出现一次的元素。
- Input: 数组
- Output: 两个元素
- Sample: [1,2,1,3,2,5], return [3,5]
- Analysis: 如果仅仅使用异或运算来对所有数据进行过滤，最终异或结果是两个目标元素的异或结果，为了能够将这两个元素区分开来，可以作如下考虑：两个元素不同，那么一定有一个bit是不同的，在异或结果中这个不同的bit就是1，只要找到这个为1的bit，就可以将数组划分成两组，一组元素的特征是在该bit上为1，另一组为0，问题就简化成了Single Number I.
- Solution:
```c
class Solution {
public:
    vector<int> singleNumber(vector<int>& nums) {
        int r = 0, n = nums.size(), i = 0, last = 0;
        vector<int> ret(2, 0);
        
        for (i = 0; i < n; ++i) 
            r ^= nums[i];
        
        last = r & (~(r - 1));
        for (i = 0; i < n; ++i)
        {
            if ((last & nums[i]) != 0)
                ret[0] ^= nums[i];
            else
                ret[1] ^= nums[i];
        }
        
        return ret;
    }
};
```
## 4.15 Maximum Product of Word Lengths
- Problem: 给定一个字符串数组，找到两个没有公共字符的字符串，使得它们的长度乘积最大。如果没有，返回0。
- Input: 字符串数组
- Output: 最大乘积
- Sample: ["abcw","baz","foo","bar","xtfn","abcdef"], return 16
- Analysis:
- Solution:
```c
class Solution {
public:
    
    int maxProduct(vector<string>& words) {
        int len = words.size();
        vector<int> mask(len);
        
        for(int i = 0; i < len; i++) {
            int sz = words[i].size();
            for(int j = 0; j < sz; j++) {
                mask[i] |= (1 << (words[i][j] - 'a'));
            }
        }
        
        int ans = 0;
        for(int i = 0; i < len; i++) {
            for(int j = 0; j < i; j++) {
                if(mask[i] & mask[j]) continue;
                int p = words[i].size()*words[j].size();
                if(p > ans) ans = p;
            }
        }
        
        return ans;
    }
};
```
## 4.16 Integer Replacement
- Problem: 给定一个正整数n，如果是偶数，那么折半，否则可以加1或者减1。返回到达1的最少计算次数。
- Input: 正整数
- Output: 到达1的最少计算次数
- Sample: 8， return 3
- Analysis: 这个问题无法从先验规则上提取最优解策略，因此，一种直接的解法就是回溯法，在回溯的过程中记录已经计算过的子问题的解。
- Solution:
```c
class Solution {
private:
    unordered_map<int, int> visited;

public:
    int integerReplacement(int n) {        
        if (n == 1) { return 0; }
        if (visited.count(n) == 0) {
            if (n & 1 == 1) {
                visited[n] = 2 + min(integerReplacement(n / 2), integerReplacement(n / 2 + 1));// n为奇数时，n/2等价于(n-1)/2，而(n/2+1)等价于((n+1)/2)
            } else {
                visited[n] = 1 + integerReplacement(n / 2);
            }
        }
        return visited[n];
    }
};
```
## 4.17 Total Hamming Distance
- Problem: 计算一个数组中所有数字之间的汉明距
- Input: 整数数组
- Output: 汉明距
- Sample: [4,14,2]， return 6
- Analysis: 首先，汉明距是针对bit来计算的，如果直接对所有的数进行双重循环求解汉明距，则会偏离高效求解方向，考虑到每次计算两个数之间的汉明距都会从32个bit上比较，因此可以直接比较所有数上某一个bit的值，是0的有多少个，是1的有多少个，那么在该bit上产生的汉明距应该是count(0)*count(1)，然后迭代统计所有的bit即可。时间复杂度可从O(n^2)下降到O(n)。
- Solution:
```c
class Solution {
public:
    int totalHammingDistance(vector<int>& nums) {
        long distance = 0;
        uint32_t mask = 0x80000000;
        while(mask)
        {
            int zero = 0, one = 0;
            for(int i=0; i<nums.size(); ++i)
            {
                if(nums[i]&mask)
                    ++one;
                else ++zero;
            }
            distance += zero * one;
            mask = mask / 2;
        }
        return distance;
    }
}; 
```
