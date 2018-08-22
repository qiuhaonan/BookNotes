# [leetcode] Stack
## 1. 概念
栈式数据结构是基本数据结构之一，用途是
- 保存历史数据
- 能够记录存入数据的出现顺序
- 访问时先访问最近存入的数据，后访问较早存入的数据

用法是
- 按照元素的出现顺序做一些匹配动作
- 保存历史路径，方便状态返回
- 解决嵌套解析问题
- 模拟递归调用
- 保存未解析完的部分数据，遇到解析尾部时出栈
- 保存局部最优的值，遇到新的最优值则迭代出栈更新


## 2. 实例
### 2.1 valid Parentheses
- Problem: Given a string containing just the characters '(', ')', '{', '}', '[' and ']', determine if the input string is valid. An input string is valid if:\
Open brackets must be closed by the same type of brackets.\
Open brackets must be closed in the correct order.\
Note that an empty string is also considered valid.
- Input: a string.
- Output: true or false
- Example: "()" , true
- Analysis: 题目要求的功能是做到括号匹配，匹配的规则很清楚，匹配发生的时机是当遇到各种右括号时，去检查左边的括号是不是与其相匹配。这要求：从左往右遍历时，如果遇到的是左括号，我们应该能够按照出现顺序保存这些左括号，当出现右括号时，我们能马上找到前一个出现的左括号并判断是不是相匹配；如果匹配，那么就应该从历史数据中删除这个左括号，然后继续向后迭代，否则就返回匹配失败。如果所有的右括号都能被匹配且没有未匹配的左括号，那么就返回成功。这中间就用到栈的思想。
- Solution:
```cpp
class Solution {
public:
    bool isValid(string s) {
        vector<char> stk;
        for(auto c:s)
        {
            if(c == '(' || c == '{' || c == '[')
                stk.push_back(c);
            else 
            {
                if(stk.size() == 0)
                    return false;
                if((c == ')' && stk.back() == '(') || (c == ']' && stk.back() == '[') || (c == '}' && stk.back() == '{'))
                    stk.pop_back();
                else 
                    return false;
            }
        }
        if(stk.size() == 0)
            return true;
        return false;
    }
};
```

## 2.2 Backspace String Compare
- Problem: Given two strings S and T, return if they are equal when both are typed into empty text editors. # means a backspace character.
- Input: two strings only contain lowercase letters and '#' characters
- Output: true or false
- Example: S = "ab#c", T = "ad#c", true
- Analysis: 判断两个字符串在经过一系列的输入和撤销操作后，是否相等。撤销操作就是把前一个输入的字符删除，那么我们只需要在遍历字符串过程中能保存已经输入的有效字符，且遇到撤销操作时如果存在前一个输入的字符，删除即可，否则继续往后搜索输入。最终形成两个结果字符串，直接按一个一个字符判断是否相等。相等，则返回true，否则返回false。这中间需要出现顺序保存和删除历史数据，因此只要使用的数据结构能够模拟栈操作即可。
- Solution: 
```cpp
class Solution {
    void generate_stack(string& s, stack<char>& st)
    {
        for(auto c:s)
        {
            if(c == '#' && !st.empty())
            {
                st.pop();
            }
            else if(c != '#')
                st.push(c);
        }
    }
public:
    bool backspaceCompare(string S, string T) {
        stack<char> stack_s, stack_t;
        generate_stack(S, stack_s);
        generate_stack(T, stack_t);
        if(stack_s.size() != stack_t.size())
            return false;
        while(!stack_s.empty())
        {
            if(stack_s.top() != stack_t.top())
                return false;
            stack_s.pop();
            stack_t.pop();
        }
        return true;
    }
};
```

## 2.3 Binary Tree Zigzag Level Order Traversal
- Problem: Given a binary tree, return the zigzag level order traversal of its nodes' values. (ie, from left to right, then right to left for the next level and alternate between).
- Input: A binary tree
- Output: the zigzag level order traversal
- Example: [3,9,20,null,null,15,7], return [[3], [20,9], [15,7]]
- Analysis: 题目的功能跟树的层次遍历相似，只是稍作改动。先回忆一下，树的层次遍历该怎么做。层次遍历时，每一层的结点都必须按照从左往右的顺序保存和访问。只要保存的顺序确定了，那么访问的顺序也就确定了。因此，访问如果需要从左往右访问，那么就在保存的时候先保存左结点，后保存右结点，反之，就先保存右结点，再保存左结点。题目要求是树的层次遍历相邻的两层应该按照相反的顺序遍历，那么就需要将这两层结点区分开，否则就不知道在什么时候应该按照哪个方向进行遍历，也就是用两份数据结构来保存当前层和下一层结点, 每遍历一层，改变一次方向。这里，如果用栈来保存数据，那么应该按照访问顺序逆序入栈，如果是队列，就按照访问顺序入队。
- Solution:
```cpp
/**
 * Definition for a binary tree node.
 * struct TreeNode {
 *     int val;
 *     TreeNode *left;
 *     TreeNode *right;
 *     TreeNode(int x) : val(x), left(NULL), right(NULL) {}
 * };
 */
class Solution {
public:
    vector<vector<int>> zigzagLevelOrder(TreeNode* root) {
        stack<TreeNode*> layer;
        if(root != nullptr)
            layer.push(root);
        vector<vector<int>> res;
        int direction = 1;
        while(layer.size()!=0)
        {
            vector<int> num;
            stack<TreeNode*> next_layer;
            while(layer.size() != 0)
            {
                if(direction == 1)
                {
                    auto node = layer.top();
                    num.push_back(node->val);
                    if(node->left != nullptr)
                        next_layer.push(node->left);
                    if(node->right != nullptr)
                        next_layer.push(node->right);
                    layer.pop();
                }
                else
                {
                    auto node = layer.top();
                    num.push_back(node->val);
                    if(node->right != nullptr)
                        next_layer.push(node->right);
                    if(node->left != nullptr)
                        next_layer.push(node->left);
                    layer.pop();
                }
            }
            layer.swap(next_layer);
            direction = direction * (-1);
            res.push_back(num);
        }
        return res;
    }
};
```

## 2.4 Verify Preorder Serialization of a Binary Tree
- Problem: One way to serialize a binary tree is to use pre-order traversal. When we encounter a non-null node, we record the node's value. If it is a null node, we record using a sentinel value such as #. \
 ________________     9 \
 ___________   /  _________ \\ \
___________3 __________     2 \
  _________/ ___\\ _ ____ / ___\\ \
 ________4 ___  1 ____ #  ____6 \
_______/ _\\ __/ _\\ _________/ \\ \
______\# __\# \# _\#   ________\# \# \
For example, the above binary tree can be serialized to the string "9,3,4,#,#,1,#,#,2,#,6,#,#", where # represents a null node.\
Given a string of comma separated values, verify whether it is a correct preorder traversal serialization of a binary tree. Find an algorithm without reconstructing the tree.\
Each comma separated value in the string must be either an integer or a character '#' representing null pointer.\
You may assume that the input format is always valid, for example it could never contain two consecutive commas such as "1,,3".
- Input: a string representing a binary tree
- Output: true or false
- Example: "9,3,4,#,#,1,#,#,2,#,6,#,#", return true
- Analysis: 题目要求判断给定的字符串序列是否是二叉树的前序遍历结果。什么时候不是前序遍历结果呢？也就是说遍历的顺序不符合二叉树的规则，那么怎么判断符不符合呢？把它当做二叉树遍历，出现叶子节点为空而不是#或者遍历完了但是字符串序列有剩余字符。实际上题目说的意思是给定一个前序遍历结果，按照前序遍历的方法去遍历它，在遍历的过程中判断是不是符合二叉树的定义。那么怎么前序遍历呢？就用普通的递归调用。剩余的第一个问题是字符串中有恼人的逗号，而这个逗号不出现在最后一个位置，这种会出现恼人的边界检查问题，不如把它删了吧。利用replace函数来删除所有的逗号。第二个问题是，如果用递归方式来前序遍历字符串，需要对已经访问的字符进行删除，否则在递归返回时无法知道下一个该访问的结点对应的字符索引，因此可以使用字符流来自动记录下一个要访问的字符。
- Solution:
```cpp
class Solution {
    int preorder_traverse(istringstream& preorder_stream)
    {
        if(preorder_stream.eof() == true)
            return -1;
        string str;
        preorder_stream>>str;
        if(str != "#")
        {
            int l = preorder_traverse(preorder_stream);
            int r = preorder_traverse(preorder_stream);
            if(l == 0 && r == 0)
                return 0;
            else return -1;
        }
        return 0;
    }
public:
    bool isValidSerialization(string preorder) {
        if(preorder.size() == 0)
            return true;
        vector<bool> v(preorder.size(), false);
        replace(preorder.begin(), preorder.end(), ',', ' ');
        istringstream preorder_stream(preorder);
        int res = preorder_traverse(preorder_stream);
        if(res == -1 || preorder_stream.eof() == false)
            return false;
        else return true;
    }
};
```

## 2.5 Flatten Nested List Iterator
- Problem: Given a nested list of integers, implement an iterator to flatten it. Each element is either an integer, or a list -- whose elements may also be integers or other lists.
- Input: a nested list of integers
- Output: a traverse of all integers.
- Example: [[1,1],2,[1,1]], return [1,1,2,1,1]
- Analysis: 题目给的输入vector中存在嵌套的vector，要求把这些嵌套的对象全部展开，由于不知道嵌套的层数，因此需要使用深度递归或者设定用户栈来递归拆解这些嵌套vector对象。
- Solution:
```cpp
/**
 * // This is the interface that allows for creating nested lists.
 * // You should not implement it, or speculate about its implementation
 * class NestedInteger {
 *   public:
 *     // Return true if this NestedInteger holds a single integer, rather than a nested list.
 *     bool isInteger() const;
 *
 *     // Return the single integer that this NestedInteger holds, if it holds a single integer
 *     // The result is undefined if this NestedInteger holds a nested list
 *     int getInteger() const;
 *
 *     // Return the nested list that this NestedInteger holds, if it holds a nested list
 *     // The result is undefined if this NestedInteger holds a single integer
 *     const vector<NestedInteger> &getList() const;
 * };
 */
class NestedIterator {
public:
    NestedIterator(vector<NestedInteger> &nestedList) {
        ans.clear();
        idx = 0;
        if(!nestedList.size())
            return ;
        int n = nestedList.size();
        for(int i = 0; i < n; ++ i){
            if(nestedList[i].isInteger())
                ans.push_back(nestedList[i].getInteger());
            else{
                dfs(nestedList[i].getList());
            }
        }
    }

    int next() {
        return ans[idx ++];
    }

    bool hasNext() {
        return idx < ans.size();
    }
    
private:
    void dfs(vector<NestedInteger> &nestedList){
        int n = nestedList.size();
        for(int i = 0; i < n; ++ i){
            if(nestedList[i].isInteger())
                ans.push_back(nestedList[i].getInteger());
            else{
                dfs(nestedList[i].getList());
            }
        }
    }

    vector<int> ans;
    int idx;
};
```
## 2.6 Decode String
- Problem: Given an encoded string, return it's decoded string. The encoding rule is: k[encoded_string], where the encoded_string inside the square brackets is being repeated exactly k times. Note that k is guaranteed to be a positive integer. You may assume that the input string is always valid; No extra white spaces, square brackets are well-formed, etc. Furthermore, you may assume that the original data does not contain any digits and that digits are only for those repeat numbers, k. For example, there won't be input like 3a or 2[4].
- Input: an encoded string
- Output: an decoded string
- Example: "2[abc]3[cd]ef", return "abcabccdcdcdef"； “3[a2[c]]", return "accaccacc"
- Analysis: 题目的问题类似于解析正则表达式字符串的原始字符串。由于这种表达式中存在嵌套表达式的可能，因此需要使用栈来保存尚未完全解析完毕的表达式，以待逐渐解析完整个表达式。从左向右遍历表达式字符串，没有遇到右括号就代表子表达式没有解析完毕，应该把当前的内容装入历史栈中，否则将栈倾出止第一个数字，然后按照数字将倾出的字符串叠加起来，再次入栈。继续向后遍历直到结束。结束后把栈中的字符串倾出并按逆序连接起来返回。
- Solution:
```cpp
class Solution {
public:
    string decodeString(const string& s, int& i) {
        string res;
        
        while (i < s.length() && s[i] != ']') {
            if (!isdigit(s[i]))
                res += s[i++];
            else {
                int n = 0;
                while (i < s.length() && isdigit(s[i]))
                    n = n * 10 + s[i++] - '0';
                    
                i++; // '['
                string t = decodeString(s, i);
                i++; // ']'
                
                while (n-- > 0)
                    res += t;
            }
        }
        
        return res;
    }

    string decodeString(string s) {
        int i = 0;
        return decodeString(s, i);
    }
};
```

## 2.7 Remove K Digits
- Problem: Given a non-negative integer num represented as a string, remove k digits from the number so that the new number is the smallest possible. \
Note:\
The length of num is less than 10002 and will be ≥ k.\
The given num does not contain any leading zero.
- Input: a non-negative integer num represented as a string
- Output: a string representing the smallest number 
- Example: "1432219", 3, return "1219"
- Analysis: 题目要求删除k位数字来形成最小的数。那么最小的数应该满足的条件就是从高位到低位进行尽力选择最小的数。因为选择是通过删除当前数字实现的，因此只需要当前的数字向前逐个比较，如果比之前的数字小，那么就删除之前的数字并更新剩余可删除的字符个数，直到比之前的数字大，再往后遍历或者剩余可删除的字符个数为0就返回。此处需要考虑特殊情况，即开头数字为0的字符应该删除，如果剩余字符为空就返回0，否则就返回字符。
- Solution:
```cpp
class Solution {
public:
    string removeKdigits(string num, int k) {
        string res = "";
        int n = num.size(), keep = n - k;
        for (char c : num) {
            while (k && res.size() && res.back() > c) {
                res.pop_back();
                --k;
            }
            res.push_back(c);
        }
        res.resize(keep);
        while (!res.empty() && res[0] == '0') res.erase(res.begin());
        return res.empty() ? "0" : res;
    }
};
```
## 2.8 132 Pattern
- Problem: Given a sequence of n integers a1, a2, ..., an, a 132 pattern is a subsequence ai, aj, ak such that i < j < k and ai < ak < aj. Design an algorithm that takes a list of n numbers as input and checks whether there is a 132 pattern in the list.
- Input: a sequence of n intergers
- Output: true or false
- Example: [1,2,3,4], return false
- Analysis: 题目重点找的是132中的32，根据32的属性，可以从后向前看，只要遇到前面的比后面的大，那么就有可能存在这个模式，剩余的只需要找到13模式即可。其实再把这个模式给揭开一层，它的本质就是找一个有序序列a<b<c，只不过b的索引比c大，那么就保存c的候选序列，遇到新的候选值时，把c序列末尾的分配给b，然后只要遇到比b小的数就可以做出判断了。
- Solution:
```cpp
class Solution {
public:
    bool find132pattern(vector<int>& nums) {
        int s3 = INT_MIN;
        stack<int> st;
        for( int i = nums.size()-1; i >= 0; i -- ){
            if( nums[i] < s3 ) return true;
            else while( !st.empty() && nums[i] > st.top() ){ 
              s3 = st.top(); st.pop(); 
            }
            st.push(nums[i]);
        }
        return false;
    }
};
```

## 2.9 Next Greater Element II
- Problem: Given a circular array (the next element of the last element is the first element of the array), print the Next Greater Number for every element. The Next Greater Number of a number x is the first greater number to its traversing-order next in the array, which means you could search circularly to find its next greater number. If it doesn't exist, output -1 for this number.
- Input: a circular array
- Output: the next greater number for every element
- Example: [1,2,1], return [2,-1,2]
- Analysis: 思想类似于2.11
- Solution: 暴力解法
```cpp
class Solution {
public:
    vector<int> nextGreaterElements(vector<int>& nums) {
        vector<int> result;
        for(int i=0; i<nums.size(); ++i)
        {
            int start = (i + 1) % nums.size();
            while(start != i)
            {
                if(nums[start] > nums[i])
                {
                    result.push_back(nums[start]);
                    break;
                }
                else start = (start + 1) % nums.size();
            }
            if(start == i)
                result.push_back(-1);
        }
        return result;
    }
};
```

## 2.10 Asteroid Collision
- Problem: We are given an array asteroids of integers representing asteroids in a row. For each asteroid, the absolute value represents its size, and the sign represents its direction (positive meaning right, negative meaning left). Each asteroid moves at the same speed. Find out the state of the asteroids after all collisions. If two asteroids meet, the smaller one will explode. If both are the same size, both will explode. Two asteroids moving in the same direction will never meet.
- Input: an array asteroids of integers
- Output: an array of rest asteroids
- Example: [5,10,-5], return [5,10]
- Analysis: 题目要求判断存活下来的卫星。根据规则可以从左往右判断，每次右边的卫星只可能迭代地和左边第一个发生碰撞，因此只要完成迭代检查碰撞可能性即可。
- Solution:
```cpp
class Solution {
public:
    vector<int> asteroidCollision(vector<int>& asteroids) {
        stack<int> running;
        for(int i=0; i<asteroids.size(); ++i)
        {
            auto iter = asteroids[i];
            if(running.empty())
                running.push(iter);
            else
            {
                int top = running.top();
                if(top * iter > 0 || (top < 0 && iter > 0))
                    running.push(iter);
                else 
                {
                    if(top < abs(iter))
                    {
                        running.pop();
                        --i;
                    }
                    else if(top == abs(iter))
                        running.pop();
                }
            }
        }
        vector<int> res;
        while(!running.empty())
        {
            res.push_back(running.top());
            running.pop();
        }
        reverse(res.begin(), res.end());
        return res;
    }
};
```

## 2.11 Daily Temperatures
- Problem: Given a list of daily temperatures, produce a list that, for each day in the input, tells you how many days you would have to wait until a warmer temperature. If there is no future day for which this is possible, put 0 instead. 
- Input: a list of daily temperatures
- Output: a list of how many days you would have to wait until a warmer temperature
- Example: [73, 74, 75, 71, 69, 72, 76, 73], return [1, 1, 4, 2, 1, 1, 0, 0]
- Analysis: 题目要求找到第一个比当前温度高的日子，也就是从下一天开始判断是否比当前温度更高，如果是就结束，否则继续向后迭代。这中间有一个高低顺序的维持，比如第二天的温度不高于第一天的温度，而第三天的温度高于第二天的温度，结果第三天的温度也一定高于第一天的温度。也就是说可以根据后面的结果反过来推出前面的结果。那么，就可以使用栈来保存之前尚未给出判断结果的数据，然后根据当前的结果逆向对比栈中的数据。另一种思路是：既然要站在当前向后看才能得到结果，那么反过来从后向前看就能直接知道结果，即每一天跟后一天或者嵌套向后跟比后一天大的所有天进行比较。
- Solution:
```cpp
class Solution {
public:
    vector<int> dailyTemperatures(vector<int>& temperatures) {
        vector<int> res(temperatures.size());
        stack<int> st;
        int index;
        for(int i = 0; i < temperatures.size(); i++){
            while(!st.empty() && temperatures[i] > temperatures[st.top()]){
                index = st.top();
                st.pop();
                res[index] = i - index;
            }
            st.push(i);
        }
        return res;
    }
};
```

## 2.12 Score of Parentheses
- Problem: Given a balanced parentheses string S, compute the score of the string based on the following rule: () has score 1, AB has score A + B, where A and B are balanced parentheses strings. (A) has score 2 * A, where A is a balanced parentheses string.
- Input: a string
- Output: a score
- Example: "()", return 1
- Analysis: 括号匹配的增强，加入括号匹配时的得分机制
- Solution:
```cpp
class Solution {
    void accu(stack<string>& result, long& num, long& res)
    {
        while(!result.empty() && isdigit(result.top().front()) == true)
        {
            long next = stol(result.top());
            num += next;
            result.pop();
        }
        if(result.empty())
            res += num;   
        else result.push(to_string(num));
    }
public:
    int scoreOfParentheses(string S) {
        stack<string> result;
        long res = 0;
        for(auto c:S)
        {
            if(c == '(')
                result.push(string(1,c));
            else
            {
                string content = result.top();
                int sum = 0;
                if(isdigit(content.front()) == true)
                {
                    long num = stol(content);
                    num = num * 2;
                    result.pop();
                    result.pop();
                    accu(result, num, res);
                }
                else
                {
                    result.pop();
                    long num = 1;
                    accu(result, num, res);
                }
            }
        }
        return res;
    }
};
```
