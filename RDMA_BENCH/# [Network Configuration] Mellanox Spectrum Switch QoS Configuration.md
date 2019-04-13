# [Network Configuration] Mellanox Spectrum Switch QoS Configuration
## 1. 1检查某个接口的队列调度策略和交换机优先级(switch priority)到TC的映射
```bash
[configure]>> show dcb ets interface ethernet [number] 
```

## 1.2 设置某个接口的优先级调度策略
```bash
[configure]>> interface ethernet [number] traffic-class [number] dcb ets strict
[configure]>> interface ethernet [number] traffic-class [number] dcb ets wrr [number]
```

## 1.3 设置交换机优先级到TC的映射
```bash
[configure]>> interface ethernet [number] traffic-class [number] bind switch-priority [number range]
```

## 2. 1检查某个接口QoS的trust状态和PCP/DSCP到交换机优先级(switch priority)的映射
```bash
[configure]>> show qos interface ethernet [number]
```

## 2.2 设置某个接口QoS的trust级别
```bash
[configure]>> interface ethernet [number] qos trust [port/both/L2/L3]
```

## 2.3 设置某个接口的PCP/DSCP到交换机优先级的映射
```bash
[configure]>> interface ethernet [number] qos map dscp [number range] to switch-priority [number range]
[configure]>> interface ethernet [number] qos map pcp [number range] dei [number range] to switch-priority [number range]
```

## 3.1 检查某个接口的队列启用PFC情况
```bash
[configure]>> show dcb priority-flow-control interface ethernet [number]
```

## 3.2 设置某个接口上队列的PFC
```bash
[configure]>> dcb priority-flow-control priority [number range] enable
[configure]>> interface ethernet [number] dcb priority-flow-control mode on force
```

## 4.1 检查某个接口的buffer分配和映射情况
```bash
[configure]>> show buffers status interfaces ethernet [number]
```

## 4.2 设置某个接口上发送队列的buffer映射到哪个pool并设置预留缓存和共享因子
```bash
[configure]>> interface ethernet [number] egress-buffer ePort.tc[number] map pool ePool[number] reserved [number]K/M shared alpha [number]
```

## 4.3 设置某个接口上接收队列的buffer映射到哪个pool并设置缓存类型和预留大小、共享因子
```bash
[configure]>> interface ethernet [number] ingress-buffer iPort.pg[number] map pool iPool[number] type lossy reserved [number]K/M shared alpha [number]
[configure]>> interface ethernet [number] ingress-buffer iPort.pg[number] map pool iPool[number] type lossless reserved [number]K/M xoff [number]K/M xon [number]K/M shared alpha [number]
```

## 5.1 检查某个接口的接收缓存优先级组(iPort.pg)到交换机优先级(switch priority)的映射
```bash
[configure]>> show buffers details interfaces ethernet [number]
```

## 5.2 设置某个接口上接收队列到交换机优先级的映射
```bash
[configure]>> interface ethernet [number] ingress-buffer iPort.pg[number] bind switch-priority [number]
```

## 6.1 检查某个接口的拥塞控制ECN设置
```bash
[configure]>> show interfaces ethernet [number] congestion-control
```

## 6.2 设置某个接口上队列的拥塞控制ECN阈值
```bash
[configure]>> interface ethernet [number] traffic-class [number] congestion-control ecn minimum-absolute [number]K/M maximum-absolute [number]K/M 
[configure]>> interface ethernet [number] traffic-class [number] congestion-control ecn minimum-relative [number] maximum-relative [number] 
```