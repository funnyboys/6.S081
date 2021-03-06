#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sysinfo.h"
#include "syscall.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  backtrace();
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_trace(void)
{
    int mask;

    if (argint(0, &mask) < 0)
        return -1;
    myproc()->syscall_nr_mask = mask;
    return 0;
}

uint64 sys_sysinfo(void)
{
    int ret;
    uint64 info; // user pointer to struct sysinfo
    struct sysinfo k_info;
    struct proc *p;

    if (argaddr(0, &info) < 0)
        return -1;

    p = myproc();
    memset(&k_info, 0, sizeof(k_info));

    (void)ret;
#if 0
    /* print input param */
    if (p->syscall_nr_mask & (1 << SYS_sysinfo)) {
        ret = copyin(p->pagetable, (char *)&k_info, info, sizeof(k_info));
        if (ret)
            return ret;
        printf("k_info->freemem = %d\n", k_info.freemem);
        printf("k_info->nproc = %d\n", k_info.nproc);
    }
#endif

    k_info.freemem = get_free_mem();
    k_info.nproc = get_nr_proc();

    return copyout(p->pagetable, info, (char *)&k_info, sizeof(k_info));
}

uint64 sys_sigalarm(void)
{
    int ticks;
    struct proc *p = myproc();
    uint64 alarm_handler = 0;

    if (argint(0, &ticks) < 0 || argaddr(1, &alarm_handler) < 0)
        return -1;

    acquire(&p->lock);
    p->nr_alarm_ticks = ticks;
    p->alarm_handler = (uint64 (*)(void))alarm_handler;
    p->in_alarm_handler = 0;
    release(&p->lock);

    
    printf("ticks = %d, alarm_handler = %p\n", ticks, alarm_handler);

    return 0;
}

extern void resume_trapframe(struct proc *p, struct trapframe *frame);
uint64 sys_sigreturn(void)
{
    struct proc *p = myproc();

    printf("p->saved_epc = %p\n", p->saved_trapframe.epc);

    acquire(&p->lock);
    resume_trapframe(p, &p->saved_trapframe);
    p->in_alarm_handler = 0;
    p->trapframe->epc = p->saved_trapframe.epc;
    release(&p->lock);

    return 0;
}
