# Deconstructing RDMA-enabled Distributed Transactions: Hybrid is Better!

## 摘要

这篇论文使用OLTP框架对不同的RDMA原语结合多种优化策略进行系统性比较。我们在单个优化过的执行框架上使用已有的和我们自己提出的新的优化策略来实现和比较不同的RDMA原语。然后对多种事务按阶段分析的方式，通过比较不同的RDMA原语来研究OCC的实现。我们的结果表明：没有单个原语能在所有的阶段胜过其他原语。我们还在相同的代码基础上对先前已有的设计进行端到端的比较，发现没有任何一个是最优的。基于以上研究，我们提出一个新的混合分布式事务系统，它总能在每个事务执行阶段选择最优的RDMA原语。

## 3. 执行框架
### 3.1 原语

**对称模型**：每台机器既扮演客户端又扮演服务器的角色。每台机器上，我们注册大页内存来减少RNIC的页转换缓存miss次数。

**创建QP**：我们为每个线程使用一个专用的上下文(Protection Domain)来创建QP，否则，即便每个线程使用自己的QP也会在驱动中存在False Synchronization问题。根本原因是每个QP使用一个预先映射的缓存来发起MMIO，以便向网卡递交请求，但是这些缓存可能会被共享。根据Mellanox的驱动实现，缓存是从一个上下文中分配的，而每个上下文持有的缓存有限。例如，mlx4驱动使用7个专用的缓存和一个共享的缓存，这意味着如果这个上下文被用来创建超过8个QP，那么额外的QP就必须共享同一个缓存。那么即便每个线程使用独占的QP，共享缓存上的吞吐量会随着线程数量的增加而下降。这个开销来自在共享MMIO缓存上的同步。

**Two-sided 原语**：在本文中，我们使用UD QP的SEND/RECV verbs作为Two-sided实现。原因是：1.在对称环境下，UD的SEND/RECV有更好的性能，尤其对事务系统而言[参照FaSST]。2.对于小型消息而言，基于one-sided RDMA的RPC不可能比基于UD的RPC性能好，因为当负载小于等于64字节时，one-sided WRITE的峰值吞吐量为130M req/s，而RPC需要两次RDMA WRITE操作，而基于UD的SEND/RECV可以达到79M req/s。

### 3.2 优化策略和被动ACK

**Coroutine(CO)**：利用pipeline来掩盖网络时延

**Outstanding requests(OR)**：并行化post requests

**Doorbell batching(DB)**：限制是只有来自同一个QP的请求会被RNIC按照batch的方式取走。