页表项的有效位数由硬件设计者指定，riscv中为 44 + 12。


# pte的组成
pte的组成: ppn(44) + flags(10)
ppn: physical address of the page(next level)   物理地址
flags: r, w, x, v, p等信息

# 虚拟地址
xv6使用39位的虚拟地址，9 + 9 + 9 + 12。
最后一个为什么是12？
一个 page 为 4096bytes， 12bit 用来实现对 4096 的索引(2 ^ 12 = 4096)。
前面为什么是9？
页表保存在物理页中，物理页的大小是 4096 bytes，而一个pte为8 bytes，因此页表最多存储512个pte信息(2 ^ 9)。

# 虚拟地址到物理地址的翻译
xv6使用3级页表进行翻译，虚拟地址共39位，9 + 9 + 9 + 12。
二级页表：取第一个9bit作为index，将 satp 的值作为物理地址中，取到pte(pte为44 + 10)。
一级页表：取第二个9bit作为index，将上一级取到的pte中的44bit末尾添上0补成55bit作为物理地址，取到pte(44 + 10)。
0级页表：取第三个9bit作为index，将上一级取到的pte中的44bit末尾添上0补成55bit作为物理地址，取到pte(44 + 10)。
将上一级取到pte中的44bit，加上虚拟地址最后的12bit，作为最终的物理地址。


# 如何判断页表中保存的pte是指向最终的页表项还是下一级页表?


实验相关:
# A kernel page table per process
# 页表
xv6 中包含两种页表：用户态页表和内核页表。
进程运行在用户态(user mode)时，使用用户态页表，运行在内核态(kernel mode)时，使用内核页表。
默认情况下，所有进程公用一份内核页表 kernel_pagetable ，kernel_pagetable 在系统启动时通过 cpu0 初始化。
# 页表的切换
页表的切换通过修改 satp 寄存器(页表基地址寄存器)值来实现，该寄存器只能在内核下修改。
```
satp: Supervisor Address Translation and Protection (satp) Register
```
## 进程从用户态切换到内核
todo：待后续章节学习完成后补充。
## 进程从内核返回用户态
todo：待后续章节学习完成后补充。
# 内核虚拟地址空间
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

## 内核页表的初始化
### 固定映射-kvminit()
kvminit() 实现了对固定映射部分的初始化，主要包含以下几个部分：
1. 外设寄存器
包括 PLIC, CLINT, UART0, VIRTIO disk，不同的外设有其不同的物理地址。
内核通过对外设控制器的寄存器进行映射，便于系统通过虚拟地址直接访问并修改硬件状态。
2. 内核代码段(rx权限)
内核代码段从 KERNBASE 开始，到 etext 结束。(KERNBASE -> etext)
etext 的大小通过链接脚本 kernel/kernel.ld 确定，表示内核代码段的结束。
3. 内核其它部分(数据段、bss、rodata)
这部分地址从 etext 开始，大小不固定，因此 xv6 不单独将其进行映射，而是从 etext 开始直接映射到 RAM 的结束地址。
RAM 的结束地址为 PHYSTOP = KERNBASE + 128 * 1024 * 1024 ，即 xv6 使用 128M 的内存。
通过 qemu 运行 xv6 时，可以看到传递给 qemu 的参数正是 128M 。
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

## 思路
为每一个进程创建单独的内核页表。






# Simplify copyin/copyinstr的意思
xv6 分为两种页表，内核页表和用户态页表。

问题
1. 为什么内核的最低虚拟地址是 PLIC 而不是 CLINT？
piazza
/* 
* CLINT 相关地址在两个地方使用: timerinit() 和 timervec()
*    start() -> timerinit(): 初始化 timer 中断(在machine mode初始化)
*    timervec(): timer 中断处理程序(在machine mode下处理)
* 而 machine mode 不支持 paging, 直接访问物理地址
* 所以内核中不需要对 CLINT 进行映射
* 因此内核虚拟地址的低地址为 PLIC 而不是 CLINT
*/