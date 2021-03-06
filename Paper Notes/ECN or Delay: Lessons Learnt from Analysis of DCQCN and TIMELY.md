# ECN or Delay: Lessons Learnt from Analysis of DCQCN and TIMELY
## Abstract
数据中心网络，尤其是无丢包的RoCEv2网络需要高效的拥塞控制协议。DCQCN和TIMELY是两种最近提出的拥塞控制算法。这篇论文中，我们使用流模型和模拟来分析DCQCN和TIMELY的稳定性，收敛性，公平性和流完成时间。我们揭示这些协议一些令人惊讶的行为。例如，我们给出了DCQCN呈现出非单调的稳定性，并且TIMELY可以收敛到一个任意不公平的稳定状态。我们提出简单的修正和调优来确保两个协议都能收敛到稳定且公平的状态。最后，通过分析得到的经验，我们回答更广泛的问题：在数据中心网络中是否有基本原因来使用ECN或者delay作为端到端的拥塞控制机制。由于现代交换机给数据包打标记的方式和基于delay的端到端拥塞控制协议的基本限制，我们认为ECN是一个更好的拥塞信号。

## Introduction
去年(2015)，研究者提出DCQCN和TIMELY两种拥塞控制协议。虽然它们是为了在大型数据中心使用RoCEv2而设计的，但它们的关键假设并不是特定于RDMA的：基于商用以太网硬件的数据中心，没有因为拥塞而产生的丢包。DCQCN和TIMELY的区别是：DCQCN使用ECN标记作为一个拥塞信号，而TIMELY测量端到端的延迟。
在继续讨论之前，我们想要强调两件事，第一，我们的讨论限制在ECN和delay作为拥塞信号和主机端基于速率的拥塞控制，因为商用以太网交换机和网卡硬件都支持它们。那些使用硬件做更多的事情或者使用一个集中控制器的工作不在我们的讨论范围。我们也不考虑设计一个新协议同时使用ECN和delay作为拥塞信号。第二，我们的目标也不是直接对比DCQCN和TIMELY的性能。这种比较没有意义，因为两种协议都提供一些调优选项(tuning knobs)，在一个给定的具体场景，任意一个协议都可以与其他协议一起执行。取而代之，我们关注两个协议的核心行为来获得更广的视角。

我们的主要贡献和发现总结如下：

DCQCN:(1)我们扩展了DCQCN的流模型来展示DCQCN有一个唯一的定点，在这个定点上，所有的流量会收敛到它们的公平共享上。使用一个离散模型，我们也推导出收敛速度是指数级的。(2)我们展示了只要反馈延迟很低，DCQCN稳定在这个定点附近。奇怪的是，稳定性和竞争流之间的关系是非单调的，这不同于TCP的行为。DCQCN对于数量非常少或者非常大的流是稳定的，但是对于数量处于其中的流是不稳定的，尤其是当网络反馈延迟很高的时候。

TIMELY:(1)我们为TIMELY开发了一个流模型并且使用模拟来验证它。这个模型揭示了TIMELY可以有无穷的稳定点，导致任意不公平性。(2)我们提出一个简单的修正方法，叫做Patched TIMELY，并且展示了这个修正的版本是稳定的并且以指数级速度收敛到一个唯一的固定点。

ECN or Delay:(1)我们通过运行同样的流量来比较TIMELY和DCQCN的流完成时间。由于更好的公平性，稳定性和比TIMELY更好的控制队列长度，DCQCN比TIMELY表现的更好。Patched TIMELY消除了这个gap，但是仍然不比不上DCQCN的性能。我们尝试了DCQCN和TIMELY参数所有的值并且给出最佳组合。因此，性能差异不太可能是参数调优的事，而更可能是它们使用的基本信号。(2)基于以上的分析，我们解释为什么ECN是一个传输拥塞信息更好的信号。原因1是现代共享缓存交换机在egress对数据包打ECN标记，有效地把排队延迟从反馈延迟中接耦合出来，这能改善系统的稳定性。原因2是我们给出了一个十分重要的结果：对于仅仅使用delay作为反馈信号的分布式协议来说，你可以实现公平性或者是一个有保证的稳定状态延迟，但不能两者同时实现。对于一个基于ECN的协议，你可以通过使用一个PI类似的标记方式来同时实现两者。但是对于基于delay的拥塞控制是不可能的。最后，ECN信号更能适应逆向路径上的抖动，因为它只会在反馈中引入延迟而基于delay的协议抖动在反馈信号中引入延迟和噪音。

## Conclusion and Future Work
我们分析了数据中心网络中最新提出的两个拥塞控制协议的行为，也就是DCQCN和TIMELY。使用流模型和控制理论分析，我们提取出DCQCN的稳定性，这展示了相对去竞争流的数量，稳定性中存在一种奇怪的非单调性行为。我们通过packet级别的模拟验证了这种行为。我们还给出了DCQCN以指数级的速度收敛到一个固定点。在对TIMELY做类似的分析时，我们发现TIMELY协议有无穷多个固定点，这会导致不可预测的行为和没有保障的公平性。我们对TIMELY提供一个简单的修正来解决这个问题。修改后的协议是稳定的，并且快速收敛。

然而，对于两个协议而言，队列长度会随着竞争流数量的增加而增长，这会引入额外的时延。通过在交换机上使用一个PI控制器来标记数据包，我们个可以保证DCQCN的有界时延和公平性。然而，我们展示并且证明了对于基于延迟的协议的十分不确定性的结果：如果你使用延迟作为仅有的反馈信号来做拥塞控制，那么你只能获得公平性或者一个固定的有界时延中的一个，而不能同时都获得。基于一下两个事实：现代共享缓存交换机的ECN标记过程可以有效地将排队时延从反馈循环中排除，ECN对于反馈中的抖动适应性更强，我们可以得出结论：在数据中心环境中，ECN是一个更好的拥塞信号。

**Future Work**: 我们正在探索在数据中心环境为RDMA拥塞控制协议做一个类似PI的控制器，包括硬件实现。我们的分析表明DCQCN可以极大的简化，去除类似非单调稳定性这种奇怪的表现。我们也计划去分析在文章中没有提到的问题。包括多瓶颈场景，更大的现实拓扑结构和负载，以及PFC导致的PAUSES对两个协议的影响，为了捕捉这些更复杂的行为，我们将会需要开发更多的分析工具并且改善DCQCN/TIMELY的NS3仿真器的性能。
