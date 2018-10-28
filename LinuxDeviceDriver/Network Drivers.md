# [Linux Device Driver3]Network Drivers
## [引言]
网络接口(network interface)是第三个标准linux设备类别。 网络接口(network interface)在系统内的角色类似于一个挂载的块设备。一个块设备把硬盘和方法注册到内核系统中，并且通过request函数以请求的形式传输和接收数据块。类似地，一个网络接口(network interface)必须把自己注册到特定的内核数据结构中，这样以来，当数据包和外界发生交换时，这些数据结构就能够被调用。

在挂载的硬盘和交付数据包的接口(interface)之间有一些重要的区别。首先，一个磁盘是在/dev目录下以一个特殊文件的形式存在的，而一个网络接口(network interface)却没有这样的入口点。正常的文件操作(read, write等等)不能用在网络接口上，所以Unix中”一切皆文件“的方法对网络接口(network interface)就不管用了。因此，网络接口(network interface)存在它们自己的命名空间中，并且导出了一套不同的操作。

也许你会反对说当应用在使用sockets时调用了read/write系统调用，但这些调用是作用在一个软件对象上，这和作用于接口(interface)是不同的。几百个sockets可以复用在同一个物理接口上。

但是这两者之间最重要的区别是：块设备驱动仅仅响应来自内核的请求，而网络驱动需要从外界异步的接收数据包。因此，一个块设备驱动是被要求将一个数据缓冲转发给内核，而网络设备是主动请求把收到的数据包推送给内核。网络驱动的内核接口就是为这种不同的操作模式设计的。

网络驱动也必须支持一系列管理任务，比如设置地址，修改传输参数和维护流量和错误统计。网络驱动的API反应了这种需求，因此与我们目前已经见过的接口有些不同。

Linux内核的网络子系统被设计成与协议完全无关，网络层协议和硬件协议的接口都是如此。网络驱动和内核之间的交互一次只处理一个数据包，这使得协议问题完全被隐藏在驱动中，物理传输被隐藏在协议中。

网络世界中使用名词octet代表一组8bits，这是网络设备和协议能理解的最小单位。名词Byte在这个上下文中不会被使用。一个header是当一个packet经过网络子系统不同层次时添加的一组bytes(err, octets)。

## [An Example Memory-based Modularized Network Interface: snull-How snull is Designed]
最开始且最重要的设计决定是接口应该与实际的硬件无关。这种限制导致事情有些像loopback接口，snull不是一个loopback接口，它模拟和远端主机的通信来更好的演示编写一个网络驱动的任务。Linux loopback驱动实际上相当简单，你可以在drivers/net/loopback.c中找到它。snull的另一个特点是它只支持IP流量。这是接口内部工作的结果--snull不得不在主机内部模拟并且解释packets来恰当地模拟一对硬件接口。真实的接口不依赖于传输协议，并且snull的这种局限并不影响章节中的代码片段。

## [Assigning IP Numbers]
如果主机向自己的ip地址发送数据包，内核不会把数据包交给网络接口，而是使用loopback来交付数据。因此，为了能够通过snull建立一个连接，在进行数据传输是必须对源地址和目的地址进行修改。为了实现这种隐式loopback，snull接口切换源地址和目的地址的第三个octet中最低位bit值(least significant bit)，也就是说它会改变C类IP号的网络号和主机号。最终的效果就是通过接口sn0发packets到网络A后，接口sn1会受到来自网络B的packets。

## [Connecting to the Kernel]
### [Device Registration]
当一个驱动模块被加载到一个运行的内核中时，它请求资源并提供功能。驱动应该探测它的设备并且使用它的硬件位置(I/O端口和IRQ线)，但是不注册它们。网络驱动被它的模块初始化函数注册的方式不同于字符设备和块设备。因为对于网络接口，不存在等效的主要和次要编号，所以网络驱动并不请求这样的编号。代替的方式是，驱动为每一个新检测到的接口在一个全局网络设备链表中插入一个数据结构。

每个接口由一个struct net_devie项描述，定义在<linux/netdevice.h>。与许多其他内核接口类似，这个net_device结构体包含一个kobject并且被引用计数，然后通过sysfs导出。分配这个结构体有专用的内核函数
```cpp
struct net_device *alloc_netdev(int sizeof_priv, const char *name, void(*setup)(struct net_device*));
```
这里，sizeof_priv是驱动私有数据区域的大小。对于网络设备，这片区域与net_device结构体一起分配。事实上，这两个区域是分配成一个大块内存，但是驱动作者应该假装对此并不知情。name是可以被用户空间看到的接口名字，应该是一种printf-style %d的格式。内核会把%d替换成下一个可用的接口编号。最终，setup是一个初始化函数的指针，可以被调用来设置net_devie结构体剩余的部分。网络子系统提供了一系列包装了alloc_netdev的辅助函数用于不同类型的接口。最通用的是alloc_etherdev，定义在<linux/etherdevice.h>中：
```cpp
struct net_device *alloc_etherdev(int sizeof_priv);
```
这个函数使用eth%d作为名字参数来分配一个网络设备。它提供了自己的初始化函数(ether_setup)来为以太网设备在一些net_device域设置合适的参数。因此，它不接受驱动提供的初始化函数。驱动应该在成功分配该结构体之后直接执行特定于驱动的初始化。还有其他类型的辅助函数，如对于光纤通信设备的alloc_fcdev(定义在<linux/fcdevice.h>)，对于FDDI设备的alloc_fddidev(<linux/fddidevice.h>)，或者对于令牌环网络的alloc_trdev(<linux/trdevice.h>)。一旦初始化完毕，剩下的就是把结构体传给register_netdev函数。在snull中，这个调用看起来像下面的形式:
```cpp
for (i = 0; i < 2; i++)
	if((result = register_netdev(snull_devs[i])))
		printk("snull: error %i registering device \"%s\"\n", result, snull_devs[i]->name);
```
此处应该注意的是：一旦你调用了register_netdev函数，你的驱动也许就会运行在设备上，因此，除非所有的事情都已经完全被初始化完成，否则不要注册这个设备。

## [Initializing Each Device]
注意到在运行时struct net_device总是被放在一起，它不能像file_operations或者block_device_operations结构体一样在编译期进行设置。因此，在调用register_netdev之前必须完成初始化。这个net_device结构体非常大而且复杂。但幸运的是，内核通过ether_setup函数来小心处理一些以太网范围的默认设置(通过调用alloc_ehterdev函数来实现)。 因为snull使用alloc_netdev函数，因此它需要提供一个额外的初始化函数，如下：
```cpp
ether_setup(dev); /* assign some of the fields */

dev->open = snull_open;
dev->stop = snull_release;
dev->set_config = snull_config;
dev->hard_start_xmit = snull_tx;
dev->do_ioctl = snull_ioctl;
dev->get_stats = snull_stats;
dev->rebuild_header = snull_rebuild_header;
dev->hard_header = snull_header;
dev->tx_timeout = snull_tx_timeout;
dev->watchdog_timeo = timeout;
/* keep the default flags, just add NOARP */
dev->flags |= IFF_NOARP;
dev->features |= NETIF_F_NO_CSUM;
dev->hard_header_cache = NULL; /* Disable caching */
```
上面的代码对net_device结构体做了例行初始化，也就是保存了指向我们驱动函数的指针。唯一不寻常的地方是对flags设置了IFF_NOARP，它指定这个接口不能使用ARP协议。ARP是一个低层的以太网协议，它的任务是把IP地址转换成以太网MAC地址。将hard_header_cache设置成null也是同样的原因，它在该接口上禁用ARP响应的缓存。

## [Module Unloading]
卸载模块时没有很特别的事。模块清理函数简单地注销接口，执行任何需要的内部清理工作，然后释放net_device结构体，返还给系统。
```cpp
void snull_cleanup(void)
{
    int i;
    for (i = 0; i < 2; i++)
    {
        if(snull_devs[i])
        {
            unregister_netdev(snull_devs[i]);
            snull_teardown_pool(snull_devs[i]);
            free_netdev(snull_devs[i]);
        }
    }
    return ;
}
```
如果在某些地方仍然有对该结构体的引用，该结构体也许会继续存在，但是驱动代码不需要考虑这个。一旦你注销了接口，内核就再也不会调用该接口的方法。注意，我们内部清理(snull_teardown_pool)必须发生在设备注销之后，在把net_device结构体返还给系统之前。因为，一旦调用了free_netdev函数，我们就不能再引用设备或者设备的private area。

##[The net_device Structure in Detail]
###[Global Information]
```cpp
char name[IFNAMSIZ];
unsigned long state;
struct net_device *next;
int (*init)(struct net_device *dev);
```
###[Hardware Information]
```cpp
unsigned long rmem_end;
unsigned long rmem_start; 
unsigned long mem_end;
unsigned long mem_start;
```
这些域包含了设备使用的共享内存的首尾地址。如果设备有不同的发送和接收内存，那么mem域用来发送，而rmem域用来接收。rmem域从来不会被引用到驱动范围之外。按照惯例，end域会被设置，因此end-start就是板上可用内存的总量。
```cpp
unsigned long base_addr; //网络接口的I/O基址，同样也不会被内核使用
unsigned char irq;
unsigned char if_port;
unsigned char dma; //分配给设备的DMA通道，仅对串行总线有意义，除了查看信息之外不会在驱动之外使用
```
### [Interface Information]
大多数接口信息都能被ether_setup函数正确的设置，但是flags和dev_addr域是特定于设备的，必须在初始化时显式的赋值。一些非以太网接口可以使用类似于ether_setup的辅助函数。在drivers/net/net_init.c文件中导出了一些列这种函数，包括下面的:
```cpp
void ltalk_setup(struct net_device *dev);
void fc_setup(struct net_device *dev);
void fddi_setup(struct net_device *dev);
void hippi_setup(struct net_device *dev);
void tr_setup(struct net_device *dev);
```
大多数设备都能被这些辅助函数类覆盖。如果你的设备与此不同，那么你需要手动地对这些域进行赋值：
```cpp
unsigned short hard_header_len;//即在IP头部之前的硬件头部字节长度
unsigned mtu;
unsigned long tx_queue_len;//可以在设备的传送队列中排队的最大帧数量
unsigned short type;//接口的硬件类型，被ARP用作确定设备支持什么样的硬件地址
unsigned char addr_len;
unsigned char broadcast[MAX_ADDR_LEN];
unsigned char dev_addr[MAX_ADDR_LEN];//MAC地址长度和设备硬件地址，设备的硬件地址必须以特定于设备的方式从接口板上读取出来，并且拷贝到dev_addr中
unsigned short flags;
int features; //接口的flags
```
flags域是包括下面bit值的位掩码，IFF前缀代表接口标志位，有一些标志是被内核管理，另一些是在初始化时由接口设置来判断不同的功能和接口的其他特征。有效的标志可以在<linux/if.h>中找到(man ifconfig)：
```cpp
IFF_UP | IFF_BROADCAST | IFF_DEBUG | IFF_LOOPBACK | IFF_POINTOPOINT | IFF_NOARP | IFF_PROMISC | IFF_MULTICAST | IFF_ALLMULTI | IFF_MASTER | IFF_SLAVE | IFF_PORTSEL | IFF_AUTOMEDIA | IFF_DYNAMIC | IFF_RUNNING | IFF_NOTRAILERS 
NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_IP_CSUM | NETIF_F_NO_CSUM | NETIF_F_HW_CSUM | NETIF_F_HIGHDMA | NETIF_F_HW_VLAN_TX | NETIF_F_HW_VLAN_RX | NETIF_F_HW_VLAN_FILTER | NETIF_F_VLAN_CHALLENGED | NETIF_F_TSO
```

### [The Device Methods]
和字符设备与块设备一样，每个网络设备也要声明可以支持的操作。其中有些操作为NULL，另外有一些不做改变，因为ether_setup为它们设置了合理的操作。

网络接口的设备方法可以分成两组：基本的和可选的。下面是基本的方法:
```cpp
int (*open)(struct net_device *dev); //open方法应该注册任何所需要的系统资源(I/O端口，IRQ，DMA等等)
int (*stop)(struct net_device *dev);
int (*hard_start_xmit)(struct sk_buff *skb, struct net_device *dev);//启动一个packet传输，整个packet包括协议头部和所有的数据都包含在一个socket bufer中
int (*hard_header)(struct sk_buff *skb, struct net_device *dev, unsigned short type, void *daddr, void *saddr, unsigned len);//在启动传输之前，为packet从源和目的硬件地址构建硬件头部
int (*rebuild_header)(struct sk_buff *skb);//在packet传输之前，在ARP解析完成之后，重新构建硬件头部
void (*tx_timeout)(struct net_device *dev);//当一个packet无法在合理的时间内完成传输，并且假设已经错过中断或者接口已经被锁住。它应该处理问题并恢复传输。
strut net_device_stats*(*get_stats)(struct net_device *dev);
int (*set_config)(struct net_device *dev, struct ifmap *map);
```
剩下的是可选的方法：
```cpp
int weight;
int (*poll)(struct net_device *dev, int *quota); //设置接口以poll模式运行，禁用中断
void (*poll_controller)(struct net_device *dev); //在禁用中断情况下，请求驱动检查接口上的事件
int (*do_ioctl)(struct net_device *dev, struct ifreq *ifr, int cmd);
void (*set_multicast_list)(struct net_device *dev);
int (*set_mac_address)(struct net_device *dev, void *addr);
int (*change_mtu)(struct net_device *dev, int new_mtu);
int (*header_cache)(struct neighbour *neigh, struct hh_cache *hh);//存储ARP解析结果
int (*header_cache_update)(struct hh_cache *hh, struct net_device *dev, unsigned char *haddr);
int (*hard_header_parse)(struct sk_buff *skb, unsigned char *haddr);
```
### [Utility Fields]
剩下的数据域被接口用来保存一些有用的状态信息。
```cpp
unsigned long trans_start;
unsigned long last_rx; //每次传输或者受到新数据时，驱动应该更新这两个值，它们是一个jiffies值
int watchdog_timeo;//在网络层确定传输超时发生之前应该经过的最短时间
void *priv;//等价于filp->private_data，但是应该使用netdev_priv来访问
struct dev_mc_list *mc_list;
int mc_count;
spinlock_t xmit_lock;//避免多个同时调用驱动的hard_start_xmit函数
int xmit_lock_owner;//持有锁的CPU编号
```

## [Opening and Closing]
我们的驱动可以在内核启动或者模块加载时探测接口。在接口能够携带packet时，内核必须打开它并且赋予它一个地址。

当ifconfig用来给接口赋予一个地址时，它执行两个任务。首先它通过ioctl(SIOCSIFADDR)赋予地址，其次再通过ioctl(SIOCSIFFLAGS)设置dev->flag中的IFF_UP位。站在设备的角度来看，第一个任务什么也没做，因为它和设备无关，但是第二个任务调用了设备的open方法。类似的，当接口被关闭时，ifconfig使用ioctl(SIOCSIFFLAGS)来清理IFF_UP位和调用stop方法。

站在实际代码的角度来看，驱动要执行的许多任务与字符设备和块设备类似。Open请求需要的系统资源并且告诉接口开始工作，Stop关闭接口并且释放系统资源。但是，网络驱动在open时必须执行一些额外的步骤。首先，在接口能与外界通信之前，MAC地址需要从硬件设备拷贝到dev->dev_addr，而MAC地址可以在open时拷贝到device中。一旦设备准备开始发送数据，open方法应该启动接口的发送队列(就是允许它接收需要传输的packets)。内核提供了一个函数来启动该队列：
```cpp
void netif_start_queue(struct net_device *dev);
void netif_stop_queue(struct net_device *dev);
```
## [Packet Transmission]
网络接口执行的最重要的任务就是发送数据和接收数据。发送数据就是指在网络链路上传送一个packet。只要内核需要发送一个数据packet，它就会调用驱动的hard_start_transmit方法把数据放在一个外出的队列(an outgoing queue)。内核处理的每个packet都包含在一个socket buffer结构体中(struct sk_buff)，定义在<linux/skbuff.h>。即使接口与socket无关，在更高的网络层上每个网络packet属于一个socket，并且任何socket的输入/输出缓冲都是struct sk_buff结构体的链表。同样的sk_buff结构体被用作承载网络数据来穿过整个linux网络子系统，但是站在接口的角度上来看，一个socket缓冲仅仅是一个packet。指向sk_buff的指针通常被叫做skb。

传递给hard_start_xmit的socket缓存包含应该出现在物理媒介上的物理packet，具有完整的传输层头部。接口不需要对传输的数据进行修改。skb->data指向需要传输的数据，skb->len是数据的长度。snull的packet传输代码如下：
```cpp
int snull_tx(struct sk_buff *skb, struct net_device *dev)
{
     int len;
     char *data, shortpkt[ETH_ZLEN];
     struct snull_priv *priv = netdev_priv(dev);
     data = skb->data;
     len = skb->len;
     if (len < ETH_ZLEN) {
     	memset(shortpkt, 0, ETH_ZLEN);
     	memcpy(shortpkt, skb->data, skb->len);
     	len = ETH_ZLEN;
     	data = shortpkt;
     }
     dev->trans_start = jiffies; /* save the timestamp */
     /* Remember the skb, so we can free it at interrupt time */
     priv->skb = skb;
     /* actual deliver of data is device-specific, and not shown here */
     snull_hw_tx(data, len, dev);
     return 0; /* Our simple device can not fail */
}
```
此处需要注意的是，当被传输的数据包要小于底层媒介支持的最短数据包长度时的处理。snull此处是直接显式的以0填充到最小长度。hard_start_xmit成功时返回0，如果返回非0值则代表此时packet不能被传输，内核会过会再重试。在这种情况下，你的驱动应该停止队列，直到导致这种错误的情况被处理完。

## [Controlling Transmission Concurrency]
hard_start_xmit函数通过一个自旋锁保护起来防止并发调用。函数返回后也许立马又被调用。当软件通知硬件有packet需要传输但硬件传输也许尚未完成，这个函数就返回了。但是对于snull而言，这不是问题，因为在函数返回之前，packet已经被CPU传输完毕。

真实的硬件接口是异步地传输packets，并且只有有限的内存来用于存储发送的packets。当内存不够时，驱动需要告诉网络系统不能再启动任何传输，直到硬件准备接收新的数据。这种提醒是通过调用netif_stop_queue函数来完成的。一旦你的驱动不得不停止入队，那么它就必须在未来的某一刻，当它能够传输新的packet时，安排重新启动入队。为了实现该目标，它应该调用：
```cpp
void netif_wake_queue(struct net_device *dev);
```
这个函数就和netif_start_queue一样，此外，它还触发网络系统来使得它再次启动传输packets。现代网卡硬件维持一个内部队列来容纳需要传输的多个packets。这种方法使得它能获得最佳的网络性能。这种设备的网络驱动必须允许可以同时进行多个传输任务，但是无论硬件是否支持同时启动多次传输，设备的内存都可能占满。当设备可用内存达到无法容纳最大可能的packet时，驱动应该停止入队直到设备内存再次可用为止。如果你必须在hard_start_xmit之外的其他地方禁止packet传输，可以使用下面的函数:
```cpp
void netif_tx_disable(struct net_device *dev);
```
这个函数的行为很像netif_stop_queue，但是它也确保当它返回时，你的hard_start_xmit方法不会被任何CPU上调用。这个队列可以通过netif_wake_queue再次重启。

## [Transmission Timeouts]
大多数硬件驱动需要做好准备面对硬件有时无法回应的情况。接口可能会忘记它们正在干吗，或者系统可能丢失掉一个中断。对于被设计运行在个人电脑上的设备来说，这种问题是家常便饭。

许多驱动通过设置计时器来解决这种问题，如果在计时器超时时该操作仍未完成，那么一定是某些地方除了问题。网络系统本质上是一个通过许多计时器控制的复杂状态机。因此，网络层代码是检测传输超时的好地方。因而，网络驱动不需要为检测这种问题而担心。它们仅仅需要设置一个超时时间(net_device结构体中的watchdog_timeo域)。这个时间应该比足够长以满足正常传输的时延(比如网络媒介中由拥塞造成的碰撞)。

如果当前的系统时间超过了设备的计时器设定的时间，网络层最终就会调用驱动的tx_timeout方法。这个方法的任务是清除问题并且确保已经在传输中的任务能够合适的完成。尤其重要的是，驱动没有丢失网络层委托的socket缓存。

snull能够通过两个加载时间参数来模拟传输锁：
```cpp
static int lockup = 0;
module_param(lockup, int, 0);
static int timeout = SNULL_TIMEOUT;
module_param(timeout, int, 0);
```
如果驱动加载时将参数lockup设置为 n，那么每隔n个packet传输完成后，就会触发一个模拟lockup，并且watchdog_timeo也被设置为给定的timeout值，另外也会调用netif_stop_queue来阻止其他尝试传输的发生。snull的传输超时处理看起来如下：
```cpp
void snull_tx_timeout(struct net_device *dev)
{
    struct snull_priv *priv = netdev_priv(dev);
    PDEBUG("Transmit timeout at %ld, latency %ld\n", jiffies,
    jiffies - dev->trans_start);
    /* Simulate a transmission interrupt to get things moving */
    priv->status = SNULL_TX_INTR;
    snull_interrupt(0, dev, NULL);
    priv->stats.tx_errors++;
    netif_wake_queue(dev);
    return;
}
```
当一个传输超时发生时，驱动必须在接口统计数据中标记错误，并且将设备恢复到正常的状态。当超时发生在snull中时，驱动会调用snull_interrupt来填充"missing"的中断并且重启传输队列。

## [Scatter/Gather I/O]
创建一个能够在网络上传输的packet的过程涉及到组装多个数据片。packet数据通常同用户空间拷贝而来，不同网络层的头部必须添加进来。这种组装过程可能要求相当多的数据拷贝。然而，如果网络接口被设计成能够以scatter/gather I/O方式来传输packet，那么packet不需要组装到单个数据块中，许多数据拷贝操作都可以被避免掉。Scatter/gather I/O也使得可以直接从用户空间零拷贝传输网络数据。

除非在device结构体中设置了NETIF_F_SG标志位，否则内核不会传递分散的packets给hard_start_xmit方法。如果你设置了这个flag，你需要检查skb内部的'shared info'域来判断是否这个packet是由单个片段还是多个片段组成，如果是多个片段组成还需要找到它们。一个特殊的宏来访问这个特殊的信息，叫做skb_shinfo。当传输可能分片的packet时，处理的第一步如下所示：
```cpp
if(skb_shinfo(skb)->nr_flags === 0){
    /* Just use skb->data and skb->len as usual*/
}
```
这个nr_flags域告诉我们有多少个片段来组装这个packet。如果是0，那么这个packet就只有一个片段，通过访问data域就可以了。如果是非0，那么你的驱动必须传输每一个独立的片段。此时，skb结构体中的数据域指向第一个片段。这个片段的长度必须通过skb->len减去skb->data_len来计算。剩下的片段可以在shared information结构体中一个叫做frags的数组中找到，frags中每个条目都是一个skb_frag_struct结构体：
```cpp
struct skb_frag_struct {
    struct page *page;
    __u16 page_offset;
    __u16 size;
};
```
我们再一次和page结构体打交道了，而不是和内核虚拟地址。你的驱动应该循环所有片段，将每个片段映射到一次DMA传输中并且不要忘记了由skb直接指向的第一个片段。你的硬件必须组装这些片段并且把它们作为一个单独的packet来传输。注意，如果你设置了NETIF_F_HIGHDMA标志位，那么有些或者全部的片段都会位于高端内存。

## [Packet Reception]
从网络中收取数据要比传输数据更加巧妙，因为必须在一个原子环境中来分配一个sk_buff并且把它交给上层处理。网络驱动实现的可能有两种数据接收模式：中断和轮询。大多数驱动实现中断驱动的技术。高带宽的适配器的驱动也许会实现轮询技术。

snull的实现将硬件细节与硬件无关的事务隔离开来。因此，在硬件收到packet之后，snull中断处理就会调用snull_rx函数，此时，packet已经在计算机的主存中。snull_rx收到一个packet的数据和它的长度。它仅有的任务就是把packet和额外的信息发送给网络代码的上层。这个代码与数据指针和长度获得的方式无关。
```cpp
void snull_rx(struct net_device *dev, struct snull_packet *pkt)
{
    struct sk_buff *skb;
    struct snull_priv *priv = netdev_priv(dev);
    /*
    * The packet has been retrieved from the transmission
    * medium. Build an skb around it, so upper layers can handle it
    */
    skb = dev_alloc_skb(pkt->datalen + 2);
    if (!skb) {
        if (printk_ratelimit( ))
        printk(KERN_NOTICE "snull rx: low on mem - packet dropped\n");
        priv->stats.rx_dropped++;
        goto out;
    }
    memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);
    /* Write metadata, and then pass to the receive level */
    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
    priv->stats.rx_packets++;
    priv->stats.rx_bytes += pkt->datalen;
    netif_rx(skb);
    out:
    return;
}
```
第一步是分配一个缓冲来保存packet。注意缓冲分配函数(dev_alloc_skb)需要知道数据长度。函数使用这些信息来为缓冲分配空间，dev_alloc_skb以原子优先级来调用kmalloc，因此在中断时可以安全地使用它。dev_alloc_skb的返回值必须被检查。在我们报告错误之前调用printk_ratelimit。但是每秒产生成百上千的命令行信息是把系统搞崩并把真正的错误隐藏起来的好方法。当太多错误信息被输出到命令行上时，printk_ratelimit通过返回0来阻止这个问题，并且事情需要被放慢速度。一旦有了一个有效的skb指针，packet数据通过memcpy拷贝到缓冲中。skb_put函数更新缓冲中的数据尾部指针并且返回一个指向新创建的空间的指针。

如果你正在为一个能把总线I/O带宽打满的接口编写高性能驱动，此处可能还有一些值得考虑的优化。一些驱动在接收数据之前就预先分配好socket缓冲，然后告诉接口把收到的数据直接放到socket的缓冲空间。网络层通过在具备DMA的内存区域分配socket缓冲来满足这种策略。这种方式可以避免拷贝操作来填充socket缓冲，但是要小心缓冲大小，因为你不知道下一次收到的数据大小是多少。一个change_mtu方法的实现在这种情况下也很重要，因为它允许驱动响应改变最大packet大小。

在理解packet之前，网络需要一些信息来解释。最终，在向上传递buffer之前，dev和protocol域必须被设置。你也许奇怪为什么我们已经在net_device结构体中设置了校验标志位，而在这里还需要再设置校验和状态。答案是：net_device接头体中设置的校验标志位是针对发送出去的数据包，不适用于收到的数据包。数据接收的最后一步是netif_rx，它把socket缓冲交付到上层。

## [The Interrupt Handler]
大多数硬件接口通过一个中断处理函数来控制。硬件打断处理器来通知两种可能事件中的一个：一个新的packet已经到达或者一个发送的数据包已经传输完毕。网络接口也能产生中断来发出错误、链路状态改变等等多种信号。

通常的中断例程可以通过检查物理设备上的状态寄存器来区分数据收发完成。snull接口也是以类似方式工作，但是它的状态字以软件实现并且存储在dev->priv中。一个网络接口的中断处理程序看起来像下面这样：
```cpp
static void snull_regular_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    int statusword;
    struct snull_priv *priv;
    struct snull_packet *pkt = NULL;
    /*
    * As usual, check the "device" pointer to be sure it is
    * really interrupting.
    * Then assign "struct device *dev"
    */
    struct net_device *dev = (struct net_device *)dev_id;
    /* ... and check with hw if it's really ours */
    /* paranoid */
    if (!dev)
    	return;
    /* Lock the device */
    priv = netdev_priv(dev);
    spin_lock(&priv->lock);
    /* retrieve statusword: real netdevices use I/O instructions */
    statusword = priv->status;
    priv->status = 0;
    if (statusword & SNULL_RX_INTR) {
        /* send it to snull_rx for handling */
        pkt = priv->rx_queue;
        if (pkt) {
        	priv->rx_queue = pkt->next;
        	snull_rx(dev, pkt);
        }
    }
    if (statusword & SNULL_TX_INTR) {
        /* a transmission is over: free the skb */
        priv->stats.tx_packets++;
        priv->stats.tx_bytes += priv->tx_packetlen;
        dev_kfree_skb(priv->skb);
    }
    /* Unlock the device and we are done */
    spin_unlock(&priv->lock);
    if (pkt) snull_release_buffer(pkt); /* Do this outside the lock! */
    	return;
}
```
中断处理程序的第一步是提取出指向正确net_device结构体的指针。这个指针通常来自收到的dev_id参数。这个中断处理程序处理传输完成的情况是最有意思的地方。在这种情况下，更新统计信息，调用dev_kfree_skb来返回不再需要的socket缓冲给系统。事实上，这个函数有三种不同的变体:
```cpp
dev_kfree_skb(struct sk_buff *skb); //当你知道你的代码此时不在中断上下文中时，可以调用该函数，snull因为没有实际的硬件中断，因此可以使用这个
dev_kfree_skb_irq(struct sk_buff *skb);//在中断上下文中使用
dev_kfree_skb_any(struct sk_buff *skb);//不确定运行的环境时使用
```

## [Receive Interrupt Mitigation]
对于高带宽接口而言，每次收到数据都产生一次中断的话会对性能造成非常大的开销。作为一种在高端系统上改善linux性能的方式，网络子系统开发者创建了一个可替代的基于轮询的接口(叫做NAPI)。在驱动开发者中，轮询是一个禁忌字眼，他们通常认为轮询技术不优雅且不高效。的确，当没有任务需要处理时对接口进行轮询是低效的。对于一个拥有高速接口并且处理大型流量的系统而言，总是有许多packets需要处理。此时，没有必要来打断处理器，以轮询方式频繁地从接口接收packets是足够的。

停止接收中断可以为处理器减少许多负担。NAPI兼容的驱动在packets由于拥塞被网络代码丢掉时也可以不必把packets交给内核，当很需要这种帮助的时候，它也可以改善性能。由于不同的原因，NAPI驱动也不太可能对packets进行重排序。
