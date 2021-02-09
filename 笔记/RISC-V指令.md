# 指令
## la
```
la rd, symbol
x[rd] = &symbol
```
将 symbol 的地址加载到 x[rd] 中。
```
la sp, stack0
```
将 stack0 的地址加载到 sp 中。
## li
```
li rd, immediate
x[rd] = immediate
```
将常量 immediate 加载到 x[rd] 中。
## csrr
`csrr csr, rs1`
读 rs1 寄存器的值到 csr 中。
## csrw
`csrw csr, rs1`
写 rs1 寄存器的值到 csr 中。
# 寄存器
## stvec寄存器
supervisor trap vector base address register
发生trap时，程序要跳转的地址。
## sepc寄存器
supervisor exception program counter
发生trap时，保存的发生异常的地址，类似arm64中的elr寄存器。
## scause寄存器
supervisor cause register
保存了发生trap的原因。
## sscratch寄存器
Spervisor Scratch Register
用户态保存内核栈的地址，内核态下为0。
## sstatus寄存器
保存cpu状态，包括中断是否使能等。
### SPP bit
The SPP bit indicates the privilege level at which a hart was executing before entering supervisor
mode. 
SPP = 0：trap from usermode
SPP = 1: otherwise
## tp寄存器
thread pointer
## satp寄存器
Supervisor Address Translation and Protection (satp) Register