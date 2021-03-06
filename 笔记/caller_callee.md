# caller
caller是调用者。
caller saved register是指由调用者保存的寄存器值。
caller saved register 在函数过程中可以随时被修改修改，函数执行完成之后，由调用者将其压栈和出栈。
# callee
callee是被调用的实体。
callee saved register是指由被调用实体保存的寄存器值。
被调用的函数来保证函数执行前和函数执行之后，寄存器值保持不变。
这种寄存器通常会在多个调用中使用。
```
参考：
https://blog.csdn.net/l919898756/article/details/103142439
```
