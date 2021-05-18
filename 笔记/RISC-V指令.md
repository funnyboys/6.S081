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

## ld
```
ld t0, 112(a0)
t0 = *(a0 + 112)
```
将 a0 + 112 的值 load 到寄存器 t0 中

## sd
```
sd ra, 0(a0)
*(a0 + 0) = ra
```
将ra的值保存到 a0 + 0 地址中

## sb
SW，SH 和 SB 指令从寄存器 rs2 的低位取出 32 位，16 位和 8 位的值保存到存储器。

## lui
LUI (Load upper immediate)
` lui rd, uimm20  `
将立即数 uimm20 左移 12 bit，低 12bits 补零，将最终的结果放到 rd 寄存器中。
例如：
```
// a5 = 0x8001 << 12 = 0x8001000
lui a5,0x8001
```

## slli
逻辑左移
例如：
```
// a5 = 0x800_1000
// a5 = a5 << 8 = 0x8_0010_0000
slli a5,a5,0x8
```

## csrr
`csrr csr, rs1`
读 rs1 寄存器的值到 csr 中。

## csrw
`csrw csr, rs1`
写 rs1 寄存器的值到 csr 中。

## csrrw
CSR read and write
```
csrrw rd, csr, rs1
  rd = csr
  csr = rs1
```
读写操作，将csr的值写入rd, 将rs1的值写入csr

## auipc
```
auipc	a0, 0x1
    //a0 = pc + (0x1 << 12)
```
用立即数构建一个偏移量的高20位，低12位填0，并将此偏移加到pc上，将最终结果存入a0。

## jalr
```
jalr	-1568(ra)
	// jump to (ra - 1568)
    junmp and link register
```
通过有符号立即数加上ra，作为目标地址进行跳转

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

# 其它
## 在汇编中使用label地址
使用 la 指令。
```
// 将 _data_start 的地址放到 a5 寄存器中
la a5, _data_start
```

## 汇编中定义变量
