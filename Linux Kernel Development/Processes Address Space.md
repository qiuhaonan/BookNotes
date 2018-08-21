# 13. 进程地址空间
Linux操作系统采用虚拟内存来管理用户空间进程的内存。也就是说系统中所有进程之间以虚拟方式共享内存。进程地址空间由每个进程中的线性地址区(一个独立的连续区间)组成。通常情况下，每个进程都有唯一的地址空间，而且进程地址空间之间彼此互不相干。但是进程之间也可以选择共享地址空间，这种进程被称为线程。

内存地址是一个给定的值，它要在地址空间范围之内的。地址空间中可以被访问的合法地址区间被称为内存区域，通过内核，进程可以给自己的地址空间动态地添加或减少内存区域。进程只能访问有效范围内的内存地址，另外每个地址范围也具有特定的访问属性，如只读或不可执行等属性。如果一个进程访问了不在有效范围中的地址，或以不正确的方式访问有效地址，那么内核就会终止该进程，并返回段错误信息。

## 13.1 内存描述符
内核使用内存描述符结构体表示进程的地址空间，由定义在linux/sched.h中的mm_struct结构体表示：
```c
struct mm_struct {
    struct vm_area_struct           *mmap;              //内存区域链表
    struct rb_root                  mm_rb;              //内存区域红黑树
    sturct vm_area_struct           *mmap_cache;        //最后使用的内存区域
    unsigned long                   free_area_cache;
    pgd_t                           *pgd;               //页全局目录
    atomic_t                        mm_users;           //用户
    atomic_t                        mm_count;           //主使用计数
    int                             map_count;          //内存区域数目
    struct rw_semaphore             mmap_sem;           //内存区域信号量
    spinlock_t                      page_table_lock;    //页表锁
    struct list_head                mmlist;             //包含全部mm_struct的链表
    unsigned long                   start_code;
    unsigned long                   end_code;
    unsigned long                   start_data;
    unsigned long                   end_data;
    unsigned long                   start_brk;
    unsigned long                   brk;
    unsigned long                   start_stack;
    unsigned long                   arg_start;
    unsigned long                   arg_end;
    unsigned long                   env_start;
    unsigned long                   env_end;
    unsigned long                   rss;                //物理页
    unsigned long                   total_vm;           //全部页面数量
    unsigned long                   locked_vm;          //上锁的页面数量
    unsigned long                   def_flags;
    unsigned long                   cpu_vm_mask;
    unsigned long                   swap_address;       //最后被扫描的地址
    unsigned                        dumpable;           //是否可以产生内存转储信息
    int                             used_hugetlb;       //是否使用了hugetlb
    mm_context_t                    context;            //体系结构特殊数据
    int                             core_waiters;       //内核转储等待线程
    struct completion               *core_startup_done; 
    struct completion               core_done;
    rwlock_t                        ioctx_list_lock;  
    struct kioctx                   *ioctx_list;
    struct kioctx                   default_kioctx;


};
```
### 13.1.1 分配内存描述符
在进程的进程描述符中，mm域存放着该进程使用的内存描述符，所以current->mm便指向当前进程的内存描述符。在创建进程时，fork()函数利用copy_mm()函数复制父进程的内存描述符，也就是current->mm域给其子进程，此时父子进程共享内存地址空间，否则子进程需要重新分配mm_struct结构体，这是通过文件kernel/fork.c中的allocate_mm()宏从mm_cachep slab缓存中分配得到的。通常，每个进程都有唯一的mm_struct，即唯一的进程地址空间。

### 13.1.2 销毁内存描述符
进程退出时，内核调用exit_mm()函数，其中mmput()函数减少内存描述符中的mm_users用户计数，如果用户计数降到0，继续调用mmdrop()函数，减少mm_count使用计数。如果使用计数也降为0，就调用free_mm()宏通过kmem_cache_free()函数将mm_struct结构体归还到mm_cachep slab缓存中。

### 13.1.3 mm_struct与内核线程
内核线程没有进程地址空间，也没有相关的内存描述符，因为内核线程并不需要访问任何用户空间的内存，而且内核线程在用户空间中没有任何页，所以内核线程对应的进程描述符中mm域为空。尽管如此，访问内核内存还是需要一些数据比如页表。为了避免内核线程的内存描述符和页表浪费内存，也为了当新内核线程运行时，避免浪费处理器周期向新地址空间进行切换，内核线程将在active_mm域中保存并直接使用前一个进程(切换前)的内存描述符，并且内核线程仅仅使用地址空间中和内核内存相关的信息，这些信息的含义和普通进程完全相同。

## 13.2 内存区域
内存区域由vm_area_struct结构体描述，定义在linux/mm.h文件中，内存区域在内核中也经常被称做虚拟内存区域或VMA。vm_area_struct结构体描述了指定地址空间内连续区间上的一个独立内存范围。内核将每个内存区域作为一个单独的内存对象管理，每个内存区域都拥有一致的属性。它可以代表多种类型的内存区域：内存映射文件，进程用户控件栈等等。定义如下：
```c
struct vm_area_struct {
    struct mm_struct            *vm_mm;
    unsigned long               vm_start;          //区间首地址
    unsigend long               vm_end;            //区间尾地址
    struct vm_area_struct       *vm_next;          //VMA链表
    pgprot_t                    vm_page_prot;      //访问控制权限
    unsigned long               vm_flags;
    struct rb_node              vm_rb;             //VMA结点
    struct list_head            shared;            //映射链表
    struct vm_operations_struct *vm_ops;
    unsigned long               vm_pgoff;          //文件中的偏移量
    struct file                 *vm_file;          //被映射的文件(如果存在)
    void                        *vm_private_data;
};
```

### 13.2.1 内存区域的树型结构和链表结构
可以通过内存描述符中的mmap和mm_rb域之一访问内存区域。这两个域各自独立地指向与内存描述符相关的全体内存区域对象，也即它们包含完全相同的vm_area_struct结构体的指针，仅仅组织方法不同。

mmap域使用单独链表按照地址增长的方向连接所有的内存区域对象。mm_rb域使用红黑树连接所有的内存区域对象。链表用于需要遍历全部结点的时候，而红黑树适用于在地址空间中定位特定内存区域的时候。

可使用/proc文件系统和pmap工具来查看特定进程的内存空间和其中所含的内存区域。

### 13.2.2 操作内存区域
内核使用do_mmap()函数创建一个新的线性地址空间，但是并不一定创建了一个新的VMA，因为如果创建的地址区间和一个已经存在的地址区间相邻，并且它们具有相同的访问权限，那么就会合并，否则就会从vm_area_cachep slab缓存中创建一个新的VMA。最后do_mmap()函数都会将一个地址区间加入到进程的地址空间中。
- unsigned long do_mmap(struct file *file, unsigned long addr, unsigned long len, unsigned long prot, unsigned long flag, unsigned long offset)
如果file参数是NULL并且offset也是0，那么就代表这次映射没有和文件相关，这种情况称为匿名映射，否则就是文件映射。
- int do_munmap(struct mm_struct *mm, unsigned long start, size_t len)
从特定的进程地址空间中删除指定地址区间

## 13.3 页表
虽然应用程序操作的对象是映射到物理内存之上的虚拟内存，但是处理器直接操作的确实物理内存，所以当应用程序访问一个虚拟地址时，首先必须将虚拟地址转换成物理地址，然后处理器才能解析地址访问请求。地址的转换工作需要通过查询页表才能完成。也就是把虚拟地址分段，将每段虚地址都作为一个索引指向页表，而页表项则指向下一级别的页表或者指向最终的物理页面。Linux中使用三级页表完成地址转换(页全局目录PGD，中间页目录PMD，页表pte_t，定义在asm/page.h中)。多级页表可以节约地址转换需占用的存放空间。多数体系结构中，搜索页表的工作是由硬件完成的，并且通过翻译后缓冲器TLB来加快搜索。TLB作为一个将虚拟地址映射到物理地址的硬件缓存。
