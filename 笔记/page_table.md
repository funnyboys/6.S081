# page table相关知识
## pte的组成
在 xv6 中，pte 的有效位是 54 bit。
```
pte: ppn(44) + flags(10)
ppn: physical address of the page(next level) ，下一级的物理地址
flags: 包含 r, w, x, v, p等访问权限信息
```
插图：pte的结构组成

## 虚拟地址
xv6使用39位的虚拟地址，9 + 9 + 9 + 12。
1. 虚拟地址为什么这么划分，最后一部分为什么是12？
mmu 按照页进行索引，而页的大小为 4096 bytes，因此最后 12bit 用来实现对页内的索引(2 ^ 12 = 4096)。
2. 虚拟地址前面为什么是9？
页表保存在物理页中，页的大小是 4096 bytes，而一个pte为8 bytes，因此页表最多存储512个pte信息(2 ^ 9)。

## 物理地址
物理地址是总线访问 RAM 或 DDR 的最终实际地址，也是硬件最终看到的地址。

## 虚拟地址到物理地址的翻译
xv6使用3级页表进行翻译，虚拟地址共39位，9 + 9 + 9 + 12。
二级页表：取第一个9bit作为index，将 satp 的值作为物理地址中，取到pte(pte为44 + 10)。
一级页表：取第二个9bit作为index，将上一级取到的pte中的44bit末尾添上0补成55bit作为物理地址，取到pte(44 + 10)。
0级页表：取第三个9bit作为index，将上一级取到的pte中的44bit末尾添上0补成55bit作为物理地址，取到pte(44 + 10)。
将上一级取到pte中的44bit，加上虚拟地址最后的12bit，作为最终的物理地址。

插图：xv6地址翻译过程


## 内核虚拟地址空间
从映射方式来看，内核虚拟地址空间被划分为两部分：固定映射 + 灵活映射
1. 固定映射
固定映射是指，不管哪一个进程切换到了内核，都需要映射并访问这一部分物理地址。
固定映射部分包括：外设、内核代码段、内核rodata、内核数据段、内核bss段、free memory、trampoline。
固定映射部分，除了trampoline之外，其它物理地址和虚拟地址相等，二者一一对应。
2. 灵活映射
灵活映射是指，不同的进程切换到内核后，需要对其单独分配内存空间并进行映射。
灵活映射部分包括：内核栈kstack。
不同的进程在内核运行时，需要使用各自独立的内核栈，因此需要为每一个进程单独分配一个物理页来存放栈信息。
灵活映射部分，物理地址和虚拟地址不一定相等。

插图：内核虚拟地址空间

## 用户态进程虚拟地址空间

插图： 进程虚拟地址空间

## 如何判断页表中保存的pte是指向最终的页表项还是下一级页表?
参考：kernel/vm.c - freewalk()
```
// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void freewalk(pagetable_t pagetable)
{
    // there are 2^9 = 512 PTEs in a page table.
    for (int i = 0; i < 512; i++) {
        pte_t pte = pagetable[i];
        if ((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0) {
            // this PTE points to a lower-level page table.
            // 当前指向的是下一级页表
            uint64 child = PTE2PA(pte);
            freewalk((pagetable_t)child);
            pagetable[i] = 0;
        } else if(pte & PTE_V)
            panic("freewalk: leaf");
    }
    kfree((void*)pagetable);
}
```

## 为什么内核的最低虚拟地址是 PLIC 而不是 CLINT？
CLINT 是中断控制器的基地址，在两个地方使用: timerinit() 和 timervec()。
```
   start() -> timerinit(): 初始化 timer 中断(在machine mode初始化)
   timervec(): timer 中断处理程序(在machine mode下处理)
```
这两处对 CLINT 的访问都在 machine mode 下进行，而 machine mode 不支持 paging, 直接访问物理地址。
所以内核中不需要对 CLINT 进行映射，即内核虚拟地址的低地址为 PLIC 而不是 CLINT。

## 页表
页表提供物理地址到虚拟地址的转换功能，目前xv6 中包含两种页表：用户态页表和内核页表。
进程运行在用户态(user mode)时，使用用户态页表，运行在内核态(kernel mode)时，使用内核页表。
默认情况下，所有进程公用一份内核页表 kernel_pagetable ，kernel_pagetable 在系统启动时通过 cpu0 初始化。
```
用户态页表: p->pagetable
内核页表：  kernel_pagetable
```
## 页表的切换
页表的切换通过修改 satp 寄存器(页表基地址寄存器)值来实现，该寄存器只能在内核下修改。
```
satp: Supervisor Address Translation and Protection (satp) Register
```

### 进程从用户态切换到内核
todo：待后续章节学习完成后补充。

### 进程从内核返回用户态
todo：待后续章节学习完成后补充。


## 内核页表的初始化
### 固定映射-kvminit()
```
固定映射和灵活映射参考 内核虚拟地址空间 章节。
```
kvminit() 实现了对固定映射部分的初始化，主要包含以下几个部分：
1. 外设寄存器
包括 PLIC, CLINT, UART0, VIRTIO disk，不同的外设有不同的物理地址。
内核通过对外设控制器的寄存器进行映射，便于系统通过虚拟地址直接访问并修改硬件状态。
2. 内核代码段(rx权限)
内核代码段从 KERNBASE 开始，到 etext 结束。(KERNBASE -> etext)
etext 的大小通过链接脚本 kernel/kernel.ld 确定，表示内核代码段的结束。
```
kernel/kernel.ld
 10   . = 0x80000000;
 11
 12   .text : {
 13     *(.text .text.*)
 14     . = ALIGN(0x1000);
 15     _trampoline = .;
 16     *(trampsec)
 17     . = ALIGN(0x1000);
 18     ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
 19     PROVIDE(etext = .);
 20   }
```
3. 内核其它部分(数据段、bss、rodata)
这部分地址从 etext 开始，大小不固定，因此 xv6 不单独将其进行映射，而是从 etext 开始直接映射到 RAM 的结束地址。
RAM 的结束地址为 PHYSTOP = KERNBASE + 128 * 1024 * 1024 ，即 xv6 使用 128M 的内存。
 qemu 在运行 xv6 时，可以看到传递给 qemu 的参数正是 128M 。
```
/* -m 参数表示设置的内存大小 */
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
```
4. free memory
free memory 是内核未使用的那部分内存，这部分内存和内核其它部分从 extext 开始一起映射。
5. trampoline
trampoline 是虚拟地址最大的那一个 page ，用户态和内核态切换时，用于保存堆栈信息。

todo：待后续章节学习完成后补充。

### 灵活映射-allocproc()
在 allocproc() 中，针对当前要使用的进程，分配一个 PAGE ，同时将其映射到内核栈。

## guard page
guard page 的虚拟地址在内核栈的下一个 page ，只有读和执行权限，没有写权限，用来防止内核栈溢出。


# 实验
## A kernel page table per process
### 要求
xv6 内核页表共享，所有进程都使用同一份内核页表。
xv6 用户态页表每个进程独立，各自使用自己独立的页表。
修改 xv6 为每一个进程创建单独的内核页表。

###　思路
参考 `内核虚拟地址空间` 章节可以知道，内核页表主要包含两部分内容：固定映射 + 灵活映射。
固定映射的部分每个进程相同，灵活映射的部分需要每个进程单独进行处理。
因此，思路如下：
1. 所有独立的内核页表都需要实现固定映射部分；
2. 创建进程分配内核栈时，需要使用对应的内核页表单独进行映射。

### 容易出错点
1. scheduler() 在 swtch() 之后需要恢复使用公用的页表
```
kvminithart(p->k_pagetable);

swtch(&c->context, &p->context);

/*
 * scheduler()只在cpu启动时调用, 从 swtch() 退出后
 * 只有公用的 kernel_pagetable 是有效的
 */
kvminithart(kernel_pagetable);
```

## Simplify copyin/copyinstr
### 要求
将进程的用户态地址空间也映射到内核页表中，保证内核能直接访问用户态空间。

###　思路
进程的用户态虚拟地址空间范围为： 0 -> p->sz
在修改 p->sz 的几个地方需要同时修改内核页表的映射：
1. exec()
exec() 会根据传入的参数，重新覆盖掉用户态地址空间。
2. fork()
fork() 需要对地址空间进行拷贝。
3. growproc()
growproc() 会分配或释放进程的虚拟地址空间， p->sz 的值同时改变，因此需要同时增加或取消对该段虚拟地址空间的映射。
### 容易出错点
