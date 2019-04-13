# [Leetcode]Backtracking
## 1.简介
计算机求解问题的算法中，暴力算法属于最简单直接的算法，但是复杂度非常高，为了降低计算复杂度，人们发现如果对问题的解空间按照一定结构进行安排，然后在此结构上按照一定策略进行搜索，能够降低暴力算法的复杂度。回溯算法就具有这种特性，它表达的是在包含问题的所有解的解空间中，把解空间按照树状结构展开，在解空间树中按照深度优先搜索的策略，从根节点出发深度搜索解空间树，当探索到某一结点时，要先判断该结点是否包含问题的解，如果包含，就从该结点出发继续探索下去，如果该结点不包含问题的解，则逐层向其祖先结点回溯。若用回溯法求解问题的所有解时，需要回溯到根，且根结点的所有可行的子树都要已被搜索遍才结束。而若使用回溯法求任一个解时，只要搜索到问题的一个解就可以结束。

## 2.问题定义
回溯算法适合解决的问题有明显的特征，通常这种问题还具有不同的目标值，分类如下：
- 组合问题
  - 在约束条件下，有多种达到目标的方法和途径，这类问题往往呈现出具有多个可能解，找出任意一个可行解或者所有的可行解，构成组合问题
    - 子集树问题(子集和问题，相容活动问题)
      - 对应的搜索树是一颗完全二叉树的组合问题
    - 排列树问题(n-皇后问题，汉密尔顿回路问题)
      - 解向量的第k个分量只能选取前k-1个分量尚未取到过的列号值
      - 一个完整的解向量恰好是输入的一个排列
- 组合优化问题
  - 如果组合问题中的每一个可能解都对应一个目标值，希望寻求合法解中对应目标值最小的或者最大的，则构成一个组合优化问题。
    - 典型问题
      - 0-1背包问题，活动选择问题，旅行商问题

## 3.问题求解框架
- 组合问题
  - 思想
    - 按照深度优先搜索策略对解空间进行递归搜索
  - 算法框架
    ```c++
    backtracker(x1, x2, x3, ... , xn)
    {
        solutions ← ∅  // 初始化可行解为空集
        k ← 1           // 初始化所在解空间树的层次为第一层
        while k >= 1:   // 保证回溯在树的根结点以下，当回溯到根结点0时就会跳出循环  
            while xk is not exhausted: //如果当前层次的候选值在解空间树中没有选择完
                do xk ← next element in xk //从解空间树这一层选取下一个候选值
                if is-partial(x1, x2, x3, ... , xk): //判断是否满足部分可行解，否则选择下一个候选值
                    if k == n: //到达解空间树最后一层
                        if is-complete(x1, x2, x3, ... , xk): //判断是否满足完整可行解
                            solutions ← solutions ∪ {x1, x2, x3, ... , xk} //将可行解加入到集合中
                    else k ← k + 1 // 解空间树向下深入一层
            reset xk //重置xk为解空间当前层中的原始值
            k ← k - 1 //解空间树向上回溯一层
        return solutions
    }
    ```
- 组合优化问题
  - 思想
    - 对于组合优化问题可以先将其视为组合问题，利用回溯算法得到所有可行解，然后计算每个可行解的目标值，最终得到最优解
    - 跟踪当前最优解的目标值most，可在回溯搜索过程中，对部分可行解x将其目标值obj与most比较，若不可能比之更优则放弃对该分支的搜索，对完整可行解x，若其目标值obj比most更优，则改写most且记录当前最优解

## 4.leetcode题目实例
- Permutations
  - Problem: Given a collection of distinct integers, return all possible permutations.
  - Input: [1,2,3]
  - Output: [[1,2,3],[1,3,2],[2,1,3],[2,3,1],[3,1,2],[3,2,1]]
  - Solutions: 排列树问题
  ```c++
  class Solution {
  public:
    vector<vector<int>> permute(vector<int>& nums) {
        vector<vector<int>> res;
        _permute(nums, res, 0); //从第0层开始深度搜索
        return res;
    }
    void swap_element(vector<int>& nums, int i, int j)
    {
        int temp = nums[i];
        nums[i] = nums[j];
        nums[j] = temp;
    }
    void _permute(vector<int>& nums, vector<vector<int>>& res, int index)
    {
        if(index==nums.size()-1) //到达最底层
        {
            res.push_back(nums); //把加入可行解
            return ;
        }
        for(int i=index; i<nums.size(); ++i) //在index层横向搜索所有可能的候选值
        {
            swap_element(nums, i, index); //使用候选值
            _permute(nums, res, index+1); //对index+1层进行深度搜索
            swap_element(nums, i, index); //还原当前值
        }
    }
  };
  ```

- PermutationsII
  - Problem: Given a collection of numbers that might contain duplicates, return all possible unique permutations.
  - Input: [1,1,2]
  - Output: [[1,1,2],[1,2,1],[2,1,1]]
  - Solutions: 非重复排列树问题
  ```c++
  class Solution {
  public:
    vector<vector<int>> permuteUnique(vector<int>& nums) {
        vector<vector<int>> res;
        _permuteUnique(nums, res, 0);
        return res;
    }
    void swap_element(vector<int>& nums, int i, int j)
    {
        int temp = nums[i];
        nums[i] = nums[j];
        nums[j] = temp;
    }
    void _permuteUnique(vector<int>& nums, vector<vector<int>>& res, int index)
    {
        if(index == nums.size()-1)
        {
            res.push_back(nums);
            return ;
        }
        unordered_set<int> dup_val;
        for(int i=index; i<nums.size(); ++i)
        {
            if(dup_val.find(nums[i]) == dup_val.end())
            {
                dup_val.insert(nums[i]);
                swap_element(nums, i, index);
                _permuteUnique(nums, res, index+1);
                swap_element(nums, i, index);
            }
        }
    }
  };
  ```
- Permutation Sequence
  - Problem: The set [1,2,3,...,n] contains a total of n! unique permutations.By listing and labeling all of the permutations in order, we get the following sequence for n = 3:["123","132","213","231","312","321"].Given n and k, return the kth permutation sequence.
  - Input: n = 3, k = 3
  - Output: "213"
  - Solutions: 精确计算排列树的搜索路径
  ```c++
  class Solution {
  public:
    string getPermutation(int n, int k) {//n代表总层次，k代表当前层次下需要找到的排列的索引
        int sum = 1;
        vector<int> permutation;
        for(int i=1; i<n; ++i)
        {
            sum = sum*i;
            permutation.push_back(i);
        }
        permutation.push_back(n);
        permute(permutation, n-1, k, 0, sum);//从第1层开始搜索
        string res = "";
        for(auto i:permutation)
            res+=to_string(i);
        return res;
    }
    void move_to_head(vector<int>& permutation, int start, int index)
    {
        int temp = permutation[start+index];
        for(int i = start + index; i> start; --i)
        {
            permutation[i] = permutation[i-1];
        }
        permutation[start] = temp;
    }
    void permute(vector<int>& permutation, int n, int k, int start, int sum)
    {
        int offset = k / sum, module = k % sum; //根据排列索引和当前层次所有排列的总数来计算所求排列在该层哪一个候选值(offset)，和排列在该候选值下所有子树路径的子索引(module)
        if(module == 0) //恰好在该层上一个候选值构成的所有子树中最后一个子索引
            --offset;
        move_to_head(permutation, start, offset);//确定该候选值对应的值
        if(module != 0) //向下一层深度搜索(start+1)，并传递下一层的子索引(module)和全部子树数目(sum/n)
            permute(permutation, n-1, module, start+1, sum/n);
        else reverse(permutation.begin()+start+1, permutation.end());//找到最终的索引，直接逆序剩下所有层的默认候选值
    }
  };
  ```
- Combinations
  - Problem: Given two integers n and k, return all possible combinations of k numbers out of 1...n.
  - Input: n = 4, k = 2
  - Output: [[1,2],[1,3],[1,4],[2,3],[2,4],[3,4]]
  - Solutions: 组合问题
  ```c++
  class Solution {
  public:
    vector<vector<int>> combine(int n, int k) {
        vector<vector<int>> res;
        vector<int> orig;
        _combine(res, orig, 1, n, k); // 从第一层开始向下搜索
        return res;
    }
    void _combine(vector<vector<int>>& res, vector<int> orig, int cur, int n, int k)
    {
        if(k == 0) //到达最底层
        {
            res.push_back(orig); //将可行解加入最终解集合中
            return ;
        }
        for(int i=cur; i<=n-k+1; ++i) //第cur层的候选值
        {
            orig.push_back(i); //将候选值添加到可行解中
            _combine(res, orig, i+1, n, k-1); //向下一层搜索
            orig.pop_back(); //将当前候选值弹出，为下一个候选值准备空间
        }
    }
  };
  ```
- Subsets
  - Problem: Given a set of distinct integers, nums, return all possible subsets (the power set). The solution set must not contain duplicate subsets.
  - Input: nums = [1,2,3]
  - Output: [[1],[2],[3],[1,2],[1,3],[2,3],[1,2,3],[]]
  - Solutions: 动态组合问题
  ```c++
  class Solution {
  public:
    vector<vector<int>> subsets(vector<int>& nums) {
        vector<vector<int>> res;
        res.push_back(vector<int>());
        for(int i=1; i<=nums.size(); ++i)
        {
            auto tmp_res = combine(nums, i);
            res.insert(res.end(), tmp_res.begin(), tmp_res.end());
        }
        return res;
    }
    vector<vector<int>> combine(vector<int>& nums, int k) {
        vector<vector<int>> res;
        vector<int> orig;
        _combine(res, orig, 0, nums, k);
        return res;
    }
    void _combine(vector<vector<int>>& res, vector<int> orig, int cur, vector<int> nums, int k)
    {
        if(k == 0)
        {
            res.push_back(orig);
            return ;
        }
        for(int i=cur; i<=nums.size()-k; ++i)
        {
            orig.push_back(nums[i]);
            _combine(res, orig, i+1, nums, k-1);
            orig.pop_back();
        }
    }
  };
  ```
- Word Search
  - Problem: Given a 2D board and a word, find if the word exists in the grid. The word can be constructed from letters of sequentially adjacent cell, where "adjacent" cells are those horizontally or vertically neighboring. The same letter cell may not be used more than once.
  - Input: board = \
    [\
        ['A', 'B', 'C', 'E']\
        ['S', 'F', 'C', 'S']\
        ['A', 'D', 'E', 'E'] \
    ]
  - Output:\
    Given word = "ABCCED", return true.\
    Given word = "SEE", return true.\
    Given word = "ABCB", return false.
  - Solutions:二维空间搜索(四叉树搜索)
  ```c++
  class Solution {
  public:
    bool exist(vector<vector<char>>& board, string word) {
        int row = board.size(), col = board.front().size();
        for(int i=0; i<row; ++i)
        {
            for(int j=0; j<col; ++j)
            {
                if(board[i][j] == word.front())//起点从整个地图上搜索
                {
                    char ch = board[i][j];
                    board[i][j] = '0';
                    bool res = _exist(board, word.substr(1), i, j);//从位置(i,j)开始进行空间深度搜索
                    board[i][j] = ch;
                    if(res == true)
                        return true;
                }
            }
        }
        return false;
    }
    bool _exist(vector<vector<char>>& board, string word, int x, int y)
    {
        if(word.size() == 0) //所有字符均匹配成功
            return true;
        char ch = board[x][y];
        board[x][y]='0';
        string next_word = word.substr(1);
        if(x-1 >= 0 && board[x-1][y] == word.front()) //向上匹配
        {
            bool res = _exist(board, next_word, x-1, y);
            if(res == true)
                return true;
        }
        if(x+1 < board.size() && board[x+1][y] == word.front()) //向下匹配
        {
            bool res = _exist(board, next_word, x+1, y);
            if(res == true)
                return true;
        }
        if(y-1 >= 0 && board[x][y-1] == word.front()) //向左匹配
        {
            bool res = _exist(board, next_word, x, y-1);
            if(res == true)
                return true;
        }
        if(y+1 < board.front().size() && board[x][y+1] == word.front()) //向右匹配
        {
            bool res = _exist(board, next_word, x, y+1);
            if(res == true)
                return true;
        }
        board[x][y] = ch;
        return false;
    }
  };
  ```
- Restore IP Addresses
  - Problem:Given a string containing only digits, restore it by returning all possible valid IP address combinations.
  - Input: "25525511135"
  - Output: ["255.255.11.135", "255.255.111.35"]
  - Solutions: 四层树搜索
  ```c++
  class Solution {
  public:
    vector<string> restoreIpAddresses(string s) {
        unordered_set<string> result;
        if(s.size() <= 12) //限制字符长度在12位以内
            _restoreIpAddresses(s, "", result, 1); //从第一层开始搜索
        return vector<string>(result.begin(), result.end());
    }
    void _restoreIpAddresses(string rest_str, string cur, unordered_set<string>& result, int seq)
    {
        if(seq == 4) //到达最后主机ip段
        {
            if(rest_str.size() != 0) //必须有数值
            {
                if(rest_str.size() != 1 && rest_str.front() == '0') //如果不止一位且开头为0则不合法
                    return ;
                int val = stoi(rest_str);
                if(val <= 255)
                {
                    cur += "." + rest_str;
                    result.insert(cur.substr(1));
                }    
            }
            return ;
        }
        for(int i=1; i<=3; ++i) //第一层可选的数字位数为1-3
        {
            if(rest_str.size() > i)
            {
                if(rest_str.front() == '0') //开头为0的必须是单个位
                {
                    auto str = rest_str.substr(0,1);
                    _restoreIpAddresses(rest_str.substr(1), cur+"."+str, result, seq+1);
                }
                else
                {
                    auto str = rest_str.substr(0, i); //开头不为0就可取多位
                    int val = stoi(str);
                    if(val <= 255) //ip地址中任意一个网段数值不能大于255
                        _restoreIpAddresses(rest_str.substr(i), cur+"."+str, result, seq+1);
                }
            }
        }
    }
  };
  ```
- Palindrome Partitioning
  - Problem: Given a string s, partition s such that every substring of the partition is a palindrome.Return all possible palindrome partitioning of s.
  - Input: "aab"
  - Output: [["aa", "b"], ["a", "a", "b"]]
  - Solutions: 组合问题
  ```c++
  class Solution {
  public:
    bool is_palindrome(string str)
    {
        if(str.size() == 1)
            return true;
        int start = 0, end = str.size()-1;
        while(start<=end)
        {
            if(str[start++]!=str[end--])
                return false;
        }
        return true;
    }
    vector<vector<string>> partition(string& s) {
        vector<vector<string>> result;
        vector<string> pool;
        _partition(s, result, pool);//从完整的字符串开始搜索
        return result;
    }
    void _partition(string rest_str, vector<vector<string>>& result, vector<string>& pool)
    {
        if(rest_str.size() == 0) //所有部分均已构成回文字符串
        {
            result.push_back(pool); //将可行解添加到解集合中
            return ;
        }
        for(int i=1; i<=rest_str.size(); ++i) //从可用字符串长度开始搜索
        {
            string str = rest_str.substr(0,i);
            if(is_palindrome(str))
            {
                pool.push_back(str);
                _partition(rest_str.substr(i), result, pool);//砍掉前面已经是回文字符串的部分，向下深度搜索
                pool.pop_back();
            }
        }
    }
  };
  ```
- Combination Sum III
  - Problem:Find all possible combinations of k numbers that add up to a number n, given that only numbers from 1 to 9 can be used and each combination should be a unique set of numbers.\
  Note:
  All numbers will be positive integers.\
  The solution set must not contain duplicate combinations.\
  - Input: k = 3, n = 7
  - Output: [[1,2,4]]
  - Solutions: 解空间坍缩(unique)的组合问题
  ```c++
  class Solution {
  public:
    vector<vector<int>> combinationSum3(int k, int n) {
        vector<vector<int>> result;
        vector<int> pool;
        _combinationSum3(1, k, n, result, pool);
        return result;
    }
    void _combinationSum3(int start, int num, int sum, vector<vector<int>>& result, vector<int>& pool)
    {
        if(num==0)
        {
            if(sum == 0)
            {
                result.push_back(pool);
            }
            return ;
        }
        for(int i=start; i<=9; ++i)
        {
            pool.push_back(i);
            _combinationSum3(i+1, num-1, sum-i, result, pool);
            pool.pop_back();
        }
    }
  };
  ```
- Beautiful Arrangement
  - Problem:Suppose you have N integers from 1 to N. We define a beautiful arrangement as an array that is constructed by these N numbers successfully if one of the following is true for the ith position (1 <= i <= N) in this array:\
  The number at the ith position is divisible by i.\
  i is divisible by the number at the ith position.\
  Now given N, how many beautiful arrangements can you construct?
  - Input: 2
  - Output: 2
  - Explanation:\
  The first beautiful arrangement is [1, 2]:\
  Number at the 1st position (i=1) is 1, and 1 is divisible by i (i=1).\
  Number at the 2nd position (i=2) is 2, and 2 is divisible by i (i=2).\
  The second beautiful arrangement is [2, 1]:\
  Number at the 1st position (i=1) is 2, and 2 is divisible by i (i=1).\
  Number at the 2nd position (i=2) is 1, and i (i=2) is divisible by 1.
  - Solutions: 具有限制条件的排列树问题
  ```c++
  class Solution {
  public:
    int countArrangement(int N) {
        int res = 0;
        vector<int> number;
        for(int i=1; i<=N; ++i)
            number.push_back(i);
        _countArrangement(N, res, 0, number);
        return res;
    }
    void swap_element(vector<int>& number, int i, int j)
    {
        int temp = number[i];
        number[i] = number[j];
        number[j] = temp;
    }
    void _countArrangement(int N, int& res, int start, vector<int>& number)
    {
        if(start == N)
        {
            ++res;
            return;
        }
        for(int i=start; i<N; ++i)
        {
            int index = start + 1, value = number[i];
            if(index % value == 0 || value % index == 0)
            {
                swap_element(number, i, start);    
                _countArrangement(N, res, start+1, number);
                swap_element(number, i, start);
            }
        }
    }
  };
  ```
- Letter Case Permutation
  - Problem:Given a string S, we can transform every letter individually to be lowercase or uppercase to create another string.  Return a list of all possible strings we could create.
  - Input: s = "a1b2"
  - Output: ["a1b2", "a1B2", "A1b2", "A1B2"]
  - Solutions: 二叉组合树问题
  ```c++
  class Solution {
  public:
    vector<string> letterCasePermutation(string S) {
        vector<string> result;
        _letterCasePermutation(S, result, 0);
        return result;
    }
    void _letterCasePermutation(string& s, vector<string>& result, int start)
    {
        if(start == s.size())
        {
            result.push_back(s);
            return ;
        }
        int i = start;
        if(i<s.size()) //可用的候选值只有本身和另一种大写或小写的形式
        {
            _letterCasePermutation(s, result, i+1);
            if(isalpha(s[i]))
            {
                if(isupper(s[i]))
                {
                    s[i]=tolower(s[i]);
                    _letterCasePermutation(s, result, i+1);
                    s[i]=toupper(s[i]);
                }
                else
                {
                    s[i]=toupper(s[i]);
                    _letterCasePermutation(s, result, i+1);
                    s[i]=tolower(s[i]);
                }
            }
        }
    }
  };
  ```
