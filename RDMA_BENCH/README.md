# [TEST SUITE] AVATAR DCT RUNNER
这个小程序可以用来测试Mellanox RDMA网卡DCT的性能(DCT简介参照Dynamically Connected Transport Service)，目前支持时延测试，使用说明如下：
```shell
$ ./a.out --help           
usage: ./a.out [options] ... 
options:
  -d, --device       device name (string [=mlx5_0])
  -s, --server       server name (string [=])
  -i, --ib           ib port (int [=1])
  -t, --tcp          tcp port (int [=18515])
  -m, --memory       memory size (int [=1048576])
  -S, --data         data size (int [=1024])
  -n, --DCT          DCT connection number (int [=1])
  -g, --gid          gid index (int [=3])
  -I, --iteration    Iteration (int [=1000])
  -M, --mode         Communication Mode (int [=0])
  -?, --help         print this message
```
## [参数说明]
- d : 设备名称，使用**ibstatus**或者**ibdev2netdev**查看
- s : 服务器IP地址，客户端使用
- i : RDMA网卡端口
- t : 服务器tcp端口
- m : 使用的注册内存大小，4096 - 1073741824之间
- S : 每次发送的数据大小
- n : 创建的DCT句柄数量
- g : gid索引
- I : 循环测试次数
- M : 模式，和n有关，当指定为0时，本地的一个DCI向远端n个DCT每个发送一次s大小的数据(n x 1)，当指定为1时，本地的一个DCI向远端1个DCT发送n次s大小的数据(1 x n)，当每次发向不同的DCT时，底层QP需要等到前面的数据都发送完并收到ACK之后再重建链接，向下一个DCT发送数据，因此会经历HOL blocking过程，时延测试可以显著观察到这个影响。

## [时延测试结果]
由于实验环境有限，只有两台机器和两张CX5网卡，因此，只给出时延测试结果。
参数设置： 1024B x 32 pipeline, 32 DCTs
- 全部发给一个DCT
```shell
Send 32 x 1024 Bytes data to 1 x DCT cost 14 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 14 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 13 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 14 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 14 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 14 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 13 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 14 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 13 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 14 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 14 us 
Send 32 x 1024 Bytes data to 1 x DCT cost 14 us 
```
- 每个都发给不同的DCT
```shell
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
Send 1 x 1024 Bytes data to 32 x DCT(s) cost 69 us 
```

可以看到，当所有的数据都发送给同一个DCT时，底层QP的流水线可以不停的工作，而当每个数据都发送到不同的DCT上面时，底层的QP每隔一个数据，流水线就要停一次，等待链接重建。
