# [Network Configuration] Enable DCQCN on 5-Generation Mellanox Adapter (ConnectX-5)
## 1.永久性方法-使用mlxconfig工具
### 1.1 安装Mellanox Firmware Tool (MFT)
通过官网http://www.mellanox.com/page/management_tools 下载MFT工具
或者已经安装MLNX_OFED驱动包的通过检查是否有mst 命令即可
### 1.2 启动MFT
```bash
# qhn @ Libra in ~/Downloads/mft-4.11.0-103-x86_64-rpm [21:13:51] 
$ sudo mst start
Starting MST (Mellanox Software Tools) driver set
Loading MST PCI module - Success
Loading MST PCI configuration module - Success
Create devices
```
### 1.3 检查设备状态
```bash
# qhn @ Libra in ~ [21:14:11] C:1
$ sudo mst status
MST modules:
------------
    MST PCI module loaded
    MST PCI configuration module loaded

MST devices:
------------
/dev/mst/mt4103_pciconf0         - PCI configuration cycles access.
                                   domain:bus:dev.fn=0000:04:00.0 addr.reg=88 data.reg=92
                                   Chip revision is: 00
/dev/mst/mt4103_pci_cr0          - PCI direct access.
                                   domain:bus:dev.fn=0000:04:00.0 bar=0xd9f00000 size=0x100000
                                   Chip revision is: 00
/dev/mst/mt4119_pciconf0         - PCI configuration cycles access.
                                   domain:bus:dev.fn=0000:42:00.0 addr.reg=88 data.reg=92
                                   Chip revision is: 00

```
### 1.4 使用mlnxconfig查询可以配置的参数
```bash
# qhn @ Libra in ~ [21:17:37] C:3
$ sudo mlxconfig -d /dev/mst/mt4119_pciconf0 q

Device #1:
----------

Device type:    ConnectX5       
Name:           N/A             
Description:    N/A             
Device:         /dev/mst/mt4119_pciconf0

Configurations:                              Next Boot
         MEMIC_BAR_SIZE                      0               
         MEMIC_SIZE_LIMIT                    _256KB(1)       
         HOST_CHAINING_MODE                  DISABLED(0)     
         HOST_CHAINING_DESCRIPTORS           Array[0..7]     
         HOST_CHAINING_TOTAL_BUFFER_SIZE     Array[0..7]     
         FLEX_PARSER_PROFILE_ENABLE          0               
         FLEX_IPV4_OVER_VXLAN_PORT           0               
         ROCE_NEXT_PROTOCOL                  254             
         ESWITCH_HAIRPIN_DESCRIPTORS         Array[0..7]     
         ESWITCH_HAIRPIN_TOT_BUFFER_SIZE     Array[0..7]     
         NON_PREFETCHABLE_PF_BAR             False(0)        
         NUM_OF_VFS                          0               
         SRIOV_EN                            True(1)         
         PF_LOG_BAR_SIZE                     5               
         VF_LOG_BAR_SIZE                     1               
         NUM_PF_MSIX                         63              
         NUM_VF_MSIX                         11              
         INT_LOG_MAX_PAYLOAD_SIZE            AUTOMATIC(0)    
         SW_RECOVERY_ON_ERRORS               False(0)        
         RESET_WITH_HOST_ON_ERRORS           False(0)        
         ADVANCED_POWER_SETTINGS             False(0)        
         CQE_COMPRESSION                     BALANCED(0)     
         IP_OVER_VXLAN_EN                    False(0)        
         PCI_ATOMIC_MODE                     PCI_ATOMIC_DISABLED_EXT_ATOMIC_ENABLED(0)
         LRO_LOG_TIMEOUT0                    6               
         LRO_LOG_TIMEOUT1                    7               
         LRO_LOG_TIMEOUT2                    8               
         LRO_LOG_TIMEOUT3                    13              
         LOG_DCR_HASH_TABLE_SIZE             11              
         DCR_LIFO_SIZE                       16384           
         ROCE_CC_PRIO_MASK_P1                255             
         ROCE_CC_ALGORITHM_P1                ECN(0)          
         CLAMP_TGT_RATE_AFTER_TIME_INC_P1    True(1)         
         CLAMP_TGT_RATE_P1                   False(0)        
         RPG_TIME_RESET_P1                   300             
         RPG_BYTE_RESET_P1                   32767           
         RPG_THRESHOLD_P1                    1               
         RPG_MAX_RATE_P1                     0               
         RPG_AI_RATE_P1                      5               
         RPG_HAI_RATE_P1                     50              
         RPG_GD_P1                           11              
         RPG_MIN_DEC_FAC_P1                  50              
         RPG_MIN_RATE_P1                     1               
         RATE_TO_SET_ON_FIRST_CNP_P1         0               
         DCE_TCP_G_P1                        1019            
         DCE_TCP_RTT_P1                      1               
         RATE_REDUCE_MONITOR_PERIOD_P1       4               
         INITIAL_ALPHA_VALUE_P1              1023            
         MIN_TIME_BETWEEN_CNPS_P1            0               
         CNP_802P_PRIO_P1                    6               
         CNP_DSCP_P1                         48              
         LLDP_NB_DCBX_P1                     False(0)        
         LLDP_NB_RX_MODE_P1                  OFF(0)          
         LLDP_NB_TX_MODE_P1                  OFF(0)          
         DCBX_IEEE_P1                        True(1)         
         DCBX_CEE_P1                         True(1)         
         DCBX_WILLING_P1                     True(1)         
         KEEP_ETH_LINK_UP_P1                 True(1)         
         KEEP_IB_LINK_UP_P1                  False(0)        
         KEEP_LINK_UP_ON_BOOT_P1             False(0)        
         KEEP_LINK_UP_ON_STANDBY_P1          False(0)        
         NUM_OF_VL_P1                        _4_VLs(3)       
         NUM_OF_TC_P1                        _8_TCs(0)       
         NUM_OF_PFC_P1                       8               
         DUP_MAC_ACTION_P1                   LAST_CFG(0)     
         SRIOV_IB_ROUTING_MODE_P1            LID(1)          
         IB_ROUTING_MODE_P1                  LID(1)          
         PCI_WR_ORDERING                     per_mkey(0)     
         MULTI_PORT_VHCA_EN                  False(0)        
         PORT_OWNER                          True(1)         
         ALLOW_RD_COUNTERS                   True(1)         
         RENEG_ON_CHANGE                     True(1)         
         TRACER_ENABLE                       True(1)         
         IP_VER                              IPv4(0)         
         BOOT_UNDI_NETWORK_WAIT              0               
         UEFI_HII_EN                         False(0)        
         BOOT_DBG_LOG                        False(0)        
         UEFI_LOGS                           DISABLED(0)     
         BOOT_VLAN                           1               
         LEGACY_BOOT_PROTOCOL                PXE(1)          
         BOOT_RETRY_CNT1                     NONE(0)         
         BOOT_LACP_DIS                       True(1)         
         BOOT_VLAN_EN                        False(0)        
         BOOT_PKEY                           0               
         EXP_ROM_UEFI_x86_ENABLE             False(0)        
         EXP_ROM_PXE_ENABLE                  True(1)         
         ADVANCED_PCI_SETTINGS               False(0)        
         SAFE_MODE_THRESHOLD                 10              
         SAFE_MODE_ENABLE                    True(1)      
```
### 1.5 启用DCQCN
首先，检查ROCE_CC_ALGORITHM_P1的值，可能的值有两个：ECN和QCN，此处我们应该设置ECN
```bash
# qhn @ Libra in ~ [21:18:37] C:4
$ sudo mlxconfig -d /dev/mst/mt4119_pciconf0 -y s ROCE_CC_ALGORITHM_P1=ECN
```
然后，我们对需要启用DCQCN的队列(traffic-class)进行设置ECN功能，例如对tc5进行设置ECN:
```bash
# qhn @ Libra in ~ [21:19:17] C:5
$ sudo mlxconfig -d /dev/mst/mt4119_pciconf0 -y s ROCE_CC_PRIO_MASK_P1=0x20
```
