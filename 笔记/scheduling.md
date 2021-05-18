# 锁和中断
持有锁为什么要关闭中断？
考虑这样一种情况：
1. 函数通过 acquire() 获取锁
2. 函数被中断打断
3. 中断也需要获取这把锁，由于锁被函数获取所以一直spin
4. 中断无法返回
5. 由于中断无法返回，函数无法继续执行释放锁
6. 死锁

# PV操作
## V
用于生产者，增加引用计数。
```
void V(struct  semaphore  *s)
{
    acquire(&s->lock);
    s->count  +=  1;
    release(&s->lock);
}
```

## P
用于消费者，等待计数非0，减少引用计数。
```
void P(struct  semaphore  *s)
{
    while(s->count  ==  0)
        ;
    acquire(&s->lock);
    s->count  -=  1;
    release(&s->lock);
}
```

# pipe
## 数据结构
```
struct pipe {
    struct spinlock lock;
    char data[PIPESIZE];
    uint nread;     // number of bytes read(不会循环计数，只会一直增大)
    uint nwrite;    // number of bytes written(同nread)
    int readopen;   // read fd is still open
    int writeopen;  // write fd is still open
};
```

## 索引pipe
index = nread/nwrite % PIPESIZE

## pipe满
nread = nwrite + PIPESIZE

## pipe空
nread = nwrite

# spinlock
## acquire

main
    scheduler
        swtch


exit

kerneltrap  usertrap
    yield

sleep
        
        
        sched
            swtch



yield:
    acquire(&p->lock)
    p->state = RUNNABLE
    sched
        swtch(&p->context, &mycpu()->context)
    release(&p->lock)