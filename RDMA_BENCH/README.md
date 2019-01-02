# [TEST SUITE] AVATAR RDMA BENCH
## 1. DCT Test
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

## 2. Copy vs Register under 4K page and 2M page Test
注册RDMA内存和拷贝数据内存的速度，决定了软件在不同性能和软件复杂度场景下面应该使用哪一种方式，是不是某一方一定比另一方好，是否在不同的系统配置下面，结果会不一样，这个小程序就探索这件事情。RDMA注册过程分为三个小步骤:
- mlock锁定物理内存
- 生成虚拟地址到物理地址的转换表
- 将这张表写到网卡缓存中去

测试中只能观测到第一个过程，因此，我们做了在不同页大小系统配置下的对比测试，结果见copy_reg_mlock.txt文件。参考文献见pdf文件。

## 3. Accelerate Transmission using Multi-queue
由于RDMA网卡处理应用的请求是通过网卡线程(Processing Unit, PU, 即DMA worker)来发起并发的DMA请求，把主机的数据拉到网卡中，对于单个QP，在一个时间段内，只会被单个网卡线程所服务，那么是否启用多个QP能够激活多个网卡线程并行处理请求，进而加速应用的数据传输呢，这个小程序可以给出相关的结果。具体代码和结果见QP_PU_test文件夹。

## 4. On-Demand-Paging Test
ODP是RDMA网卡进行内存访问的新功能，它可以允许整个进程地址空间的内存都能被网卡访问，且不需要锁定内存，也不需要代价很高的注册过程，它仅仅定义内存的访问权限。一开始的时候，网卡把所有的页面都定义为不在缓存中，发送数据时一旦找不到MTT表项，它就会产生一个ODP事件告诉操作系统，操作系统会解析需要的MTT表项并把它写到网卡缓存中，如果下一次再访问这块内存，那么就不会再产生ODP事件了。由于ODP不会锁定内存，因此操作系统在决定把某些页面换出时，会把网卡MTT缓存中的相关表项擦除。总而言之：优点是统一化内存管理，快速注册，缺点是IO页错误时会带来很高的时延。

但是，驱动还提供了另一种功能：叫做prefetch mr，就是说用户可以自定义地把MTT表项写到网卡缓存中，注意，这不是锁定内存，也不是注册，这仅仅是告诉网卡这些表项会有用，可以利用时间局部性原理来使用prefetch mr降低ODP的页错误几率。

具体的测试代码见ODP_test，用法如下:
```shell
$ ./a.out --help
usage: ./a.out [options] ... 
options:
  -d, --device       device name (string [=mlx5_0])
  -s, --server       server name (string [=])
  -i, --ib           ib port (int [=1])
  -t, --tcp          tcp port (int [=18515])
  -m, --memory       memory size (int [=2097152])
  -S, --data         data size (int [=1024])
  -n, --batch        batch size (int [=1])
  -g, --gid          gid index (int [=3])
  -I, --iteration    Iteration (int [=1000])
  -M, --mode         Communication Mode (int [=0])
  -H, --huge         Huge Page (int [=0])
  -?, --help         print this message
```