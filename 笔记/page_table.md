satp: 页表基地址寄存器

页表项的有效位数由硬件设计者指定，riscv中为 44 + 12。

pte的组成: ppn(44) + flags(10)
ppn: physical address of the page(next level)   物理地址
flags: r, w, x, v, p等信息

3级页表的翻译:
虚拟地址 9 + 9 + 9 + 12
二级页表：取第一个9bit作为index，将satp的值作为物理地址中，取到pte(pte为44 + 10)。
一级页表：取第二个9bit作为index，将上一级取到的pte中的44bit末尾添上0补成55bit作为物理地址，取到pte(44 + 10)。
0级页表：取第三个9bit作为index，将上一级取到的pte中的44bit末尾添上0补成55bit作为物理地址，取到pte(44 + 10)。
将上一级取到pte中的44bit，加上虚拟地址最后的12bit，作为最终的物理地址。

虚拟地址共 39 位，9 + 9 + 9 + 12
前面为什么是9?
页表保存在一个page size(4096 bytes)，一个pte为8 bytes，因此页表最多存储512个pte信息(2 ^ 9)。

walk能直接用物理地址替换虚拟地址，是因为xv6是直接映射，物理地址和虚拟地址相同。