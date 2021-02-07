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