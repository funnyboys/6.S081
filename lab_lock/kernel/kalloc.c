// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

int get_cpu_id()
{
   int cpu;
   push_off();
   cpu = cpuid();
   pop_off();
   return cpu;
}

// kalloc page from other cpu
void *migrate_kalloc(int cpu_id)
{
  struct run *r = 0;
  for (int i = 0; i < NCPU; i++) {
  
    if (i == cpu_id)
      continue;
  
    acquire(&cpus[i].kmem_cpu.lock);
    r = cpus[i].kmem_cpu.freelist;
    if (r) {
      cpus[i].kmem_cpu.freelist = r->next;
      release(&cpus[i].kmem_cpu.lock);
      return r;
    }
    release(&cpus[i].kmem_cpu.lock);
  }
  return 0;
}

void
kinit(int cpu_id)
{
  char name[10] = {0};
  snprintf(name, 10, "kmem%d", cpu_id);
  initlock(&cpus[cpu_id].kmem_cpu.lock, name);
  cpus[cpu_id].kmem_cpu.freelist = 0;

  if (cpu_id == 0)
    freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  int cpu_id;
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  cpu_id = get_cpu_id();
  acquire(&cpus[cpu_id].kmem_cpu.lock);
  r->next = cpus[cpu_id].kmem_cpu.freelist;
  cpus[cpu_id].kmem_cpu.freelist = r;
  release(&cpus[cpu_id].kmem_cpu.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  int cpu_id;
  struct run *r;

  cpu_id = get_cpu_id();

  acquire(&cpus[cpu_id].kmem_cpu.lock);
  r = cpus[cpu_id].kmem_cpu.freelist;
  if (r) {
    cpus[cpu_id].kmem_cpu.freelist = r->next;
    release(&cpus[cpu_id].kmem_cpu.lock);
  } else {
    release(&cpus[cpu_id].kmem_cpu.lock);
    r = migrate_kalloc(cpu_id);
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
