# RISC-V的三种cpu模式
` machine mode, supervisor mode, and user mode. `
## 模式的切换
machine mode 到 supervisor mode:
```
mret指令
跳转到mepc指向的地址(machine mode exception program counter)
```
# 内核模式
## monolithic kernel(宏内核)
操作系统管理所有的硬件敏感资源。
## microkernel
将部分系统调用放在用户态，app使用系统调用时通过进程间通信的方式来实现。
# process
进程包含两种栈(内核栈和用户栈)，二者互相独立。
RISC-VD的栈向下生长
# 切换到特权模式的方法
一共三种方法：
1. 系统调用，ecall指令
2. 异常exception，例如除0或者非法虚拟地址；
3. 设备中断。
# syscall的处理
## syscall的进入
用户态接口通过 `user/usys.pl` 定义，这里以fork()为例：
```
sub entry {
    my $name = shift;
    print ".global $name\n";
    print "${name}:\n";
    print " li a7, SYS_${name}\n";  #将系统调用号保存到a7寄存器中
    print " ecall\n";               #ecall执行进入内核
    print " ret\n";                 #从内核退出后，正常ret
}

entry("fork");
```
## 内核侧的处理
### ecall在内核的处理入口
内核启动时，通过 main() -> trapinithart() 设置了每个cpu core的异常向量表为 kernelvec 。
```
// set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
    w_stvec((uint64)kernelvec);
}

// in kernelvec.S, calls kerneltrap().
void kernelvec();
```
### kernelvec

# 内存管理
## copy_from_user的实现(fstat为例)
### fstat系统调用定义
```
int fstat(int fd, struct stat*);
```
### 内核获取用户态指针
```
syscall.c: argaddr(int n, uint64 *ip)

uint64 st; // user pointer to struct stat
if (argaddr(1, &st) < 0)
    return -1;
```
### 内核获取需要的数据
```
kernel/file.c: int filestat(struct file *f, uint64 addr)

struct stat st;
stati(f->ip, &st);  //把数据保存到st结构体中
```
### 将数据从内核拷贝到用户态
```
/*
 * p->pagetable：当前进程页表
 * addr: 用户态地址，指向struct st
 * st: 内核数据结构，保存了要获取的数据
 */
if (copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
    return -1;
```
## 内核下的malloc和free
### kfree
#### 代码实现
```
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
    struct run *r;

    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}
```
#### kfree实例
按照如下顺序分别调用三次kfree()，kmem.freelist的分布情况如下：
```
kmem.freelist -> NULL
kfree(a);
kmem.freelist -> a -> NULL
kfree(b);
kmem.freelist -> b -> a -> NULL
kfree(c);
kmem.freelist -> c -> b -> a -> NULL
```
### kalloc
#### 代码实现
```
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void)
{
    struct run *r;

    acquire(&kmem.lock);
    r = kmem.freelist;
    if(r)
        kmem.freelist = r->next;
    release(&kmem.lock);

    if(r)
        memset((char*)r, 5, PGSIZE); // fill with junk
    return (void*)r;
}
```
#### kalloc实例
按照如下顺序分别调用三次kalloc()，返回的地址以及kmem.freelist的分布情况如下：
```
kmem.freelist -> c -> b -> a -> NULL
kalloc(), 返回c
kmem.freelist -> b -> a -> NULL
kalloc(), 返回b
kmem.freelist -> a -> NULL
kalloc(), 返回a
kmem.freelist -> NULL
kalloc()，返回NULL
```
### 问题
#### 问题1
kalloc() 返回的地址包括 struct run 头信息，如果头信息被破坏了，kfree() 的时候怎么办？
kfree()中并没有使用run->next 节点，这个节点只提供给kmem.freelist来管理空闲内存，非空闲内存不会使用。
# load average
## linux
