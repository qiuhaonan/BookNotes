# [RoCE]新旧设备共存的RoCE环境搭建(搭环境两小时，跑实验五分钟)
## 1.基本配置
+ 网卡
    
    + Mellanox ConnectX-3 Pro EN 40GbE
    + Mellanox ConnectX-5 EN 100GbE
+ 交换机
    
    + Mellanox MSN2100-CB2F Spectrum 100GbE 1U Open Ethernet Switch
+ 网线

    + Mellanox 40GBASE-CR4, Passive copper
    + Mellanox 100GBASE-CR4, Passive copper
+ 服务器

    + Dell R720
    + CentOS 7.3.1611
    + Mellanox OFED 4.3-1.0.1.0
## 2.网络配置
+ 物理配置
    + 所有的网卡都通过网线接入到交换机，形成one-hop环境
+ 软件配置
    + 交换机
        + 通过命令行或者WebCLI界面将接有网卡的端口统一降速为40GbE
        + 通过命令行或者WebCLI界面在上述端口中均启用PFC功能
    + 网卡
        + 在装有ConnectX-3 Pro的机器上，设置网卡的RoCE模式为RoCEv2
	<img src=https://github.com/qiuhaonan/BookNotes/blob/master/reffig/20180718/NIC.png width="100%">
        + ConnectX-5网卡默认是RoCEv2
        + 为每个网卡设定静态局域网地址
	<img src=https://github.com/qiuhaonan/BookNotes/blob/master/reffig/20180718/IP.png width="100%">
        + 检查设备的gid，看是否有RoCEv2的gid
	<img src=https://github.com/qiuhaonan/BookNotes/blob/master/reffig/20180718/gids.png width="100%">
    + 服务器ssh免密登录
    <img src=https://github.com/qiuhaonan/BookNotes/blob/master/reffig/20180718/ssh-config.png width="100%">

## 3.分布式实验
+ 软件部署
    + 在所有结点上部署分布式程序
    + 检查程序使用的硬件参数与所在结点的环境是否一致

+ 脚本编写
<img src=https://github.com/qiuhaonan/BookNotes/blob/master/reffig/20180718/scripts.png width="100%">

## 4.总结
+ ConnectX-3 Pro的速率是40Gb，而交换机的端口速率默认是100Gb，因此，需要手动在交换机端口上进行降速，否则交换机不识别该网卡，注意，每次交换机重启都需要检查该设置
+ ConnectX-3 Pro和ConnectX-5默认的RoCE版本不一致，需要手动进行调整，否则无法互相通信
+ RDMA网卡需要设置IP且IP版本需要一致，否则无法进行通信，设置静态IP会保证IP不会丢失
+ 分布式脚本的运行需要保证所有机器都可以通过ssh免密登录，否则使用后台运行的shell命令无法提示输入密码
