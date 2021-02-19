# 启用gdb
```
窗口1:
make qemu-gdb

窗口2：
riscv64-unknown-elf-gdb kernel/kernel
```
# 快捷键
## 显示所有断点信息
```
info breakpoints
```
```
(gdb) info breakpoints
Num     Type           Disp Enb Address            What
1       breakpoint     keep y   0x0000003ffffff10e
```
## 删除断点
```
delete 断点编号
```
```
(gdb) info breakpoints
Num     Type           Disp Enb Address            What
1       breakpoint     keep y   0x0000003ffffff10e
        breakpoint already hit 2 times
2       breakpoint     keep y   0x0000000080000efa in main at kernel/main.c:13
(gdb) delete 1
(gdb) info breakpoints
Num     Type           Disp Enb Address            What
2       breakpoint     keep y   0x0000000080000efa in main at kernel/main.c:13
(gdb)
```
## 分屏模式同时显示汇编
```
layout split
```
分屏模式下切换窗口：
1. 切换到下一窗口
```
(gdb) fs next
Focus set to cmd window.
```
2. 指定切换窗口
```
(gdb) info win
Name       Lines Focus
src           38
status         1
cmd           20 (has focus)
(gdb) fs src
Focus set to src window.
```
## 打印结构体中的内容
```
set print pretty on
```