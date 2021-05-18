# 思路

1. sbrk()只改变p->sz,不实际分配物理内存和创建页表
2. usertrap()中判断为page fault且为堆地址，分配页面
3. 增加p->ustack表示用户栈起始地址，用于判断虚拟地址是否为堆地址
4. uvmcopy()、copyin()、copyout()等需做特殊处理

