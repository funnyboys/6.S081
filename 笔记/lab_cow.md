# fork中遇到的问题
fork()会拷贝父进程所有的地址空间到子进程，因此：
1. 如果父进程的地址空间很大，拷贝会非常耗时；
2. 拷贝有时候会浪费(fork()之后立即执行exec()，之前拷贝的数据会被全部覆盖)。

# 解决方案
copy-on-write fork()延缓内存页的拷贝，直到子进程真正使用时才拷贝物理内存，具体实现为：
1. fork()创建子进程时，用户态页表的pte指向父进程的物理页；
2. fork()将父子进程的pte标记为not writeable和cow page；
3. 任何一个父子进程尝试写cow page，cpu产生一个page fault；
4. page fault handler为page fault的进程分配物理内存，拷贝内容，同时修改pte指向新的页(新页为writeable)；
5. kfree()的页面需要等所有进程不再使用时才能释放。

# plan
1. 修改uvmcopy()：不分配新的物理内存、子进程的pte指向父进程的物理内存、父进程和子进程同时清除PTE_W标志；
2. 修改usertrap()：发生page fault时，使用kalloc()分配内存，拷贝旧页中的数据到新页，设置新页PTE_W标志；
3. 确保物理页在没有pte指向它时才释放；
4. 使用reference_count来追踪物理页；
5. 修改copyout()，当出现COW page fault时，使用相同方法来处理。

# hints
1. 使用PTE中的RSW(reserved for software)来标识是否为COW mapping；
2. 如果COW page fault发生时，没有空余内存，应该杀死进程。

# reference_count的管理
## 增加映射
uvmcopy() 将父进程的用户态地址空间拷贝给子进程，因此在这里需要将对应的物理页引用增加。

## 减少映射
### user trap
当发生 user trap 的页面为 cow page 时，需要将之前的页面释放掉，重新分配新的物理内存并拷贝数据。

### copyout
如果 copyout 中的用户态虚拟地址为 cow page ，需要采取与 usertrap 相同的处理方式。