#[1] 数组
##1.1 c++不支持变长数组
```cpp
int size = 10;
int arr[size]; //error, non-const length
```
##[2] lambda
##2.1 lambda无法捕捉变长变量
```cpp
fd=...
...
int size = 10;
char buf[size];
auto lambda = [&](){
    ...
    send(fd, buf, size);//error, cannout capture the variable-length variable
    ...
};
```
