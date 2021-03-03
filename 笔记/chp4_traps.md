# 出现trap的三种情况
1. 用户态主动进入
用户态程序调用系统调用，系统调用最终执行 `ecall` 指令。
2. 异常
例如：除0、访问无效虚拟地址
3. 设备中断
硬盘完成读写，发出一个完成中断。
# 重要寄存器
## stvec
supervisor trap vector base address register
发生trap时，程序要跳转的地址。
## sepc
supervisor exception program counter
发生trap时，保存的发生异常的地址，类似arm64中的elr寄存器。
trap处理完毕后，sret指令会将sepc的值加载到pc，内核从上次被中断的位置继续执行。
## scause
supervisor cause register
保存了发生trap的原因。
## sscratch
Spervisor Scratch Register
用户态保存内核栈的地址，内核态下为0。
## sstatus
cpu控制寄存器，控制中断是否使能等。
### SIE bit
SIE bit 控制中断是否使能。
### SPP bit
SPP bit 表示trap发生前的特权级别，并用来控制sret要返回的模式。
The SPP bit indicates the privilege level at which a hart was executing before entering supervisor mode. 
SPP = 0：trap from usermode
SPP = 1: otherwise
# 发生trap时硬件的处理过程
1. If the trap is a device interrupt, and the sstatus SIE bit is clear, don’t do any of the following.
2. Disable interrupts by clearing SIE.
3. Copy the pc to sepc.
4. Save the current mode (user or supervisor) in the SPP bit in sstatus.
5. Set scause to reﬂect the trap’s cause.
6. Set the mode to supervisor.
7. Copy stvec to the pc.
8. Start executing at the new pc.
硬件不会切换页表，不会切换堆栈，除了PC之外不会保存任何寄存器，所有的保存操作都需要软件来进行。