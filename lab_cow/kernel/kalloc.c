// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

int page_referance[MAX_PAGE_NR] = {0};
#define PA2PFN(pa) (((uint64)pa - KERNBASE) >> 12)

void inc_page_referance(uint64 pa)
{
    page_referance[PA2PFN(pa)]++;
}

void dec_page_referance(uint64 pa)
{
    page_referance[PA2PFN(pa)]--;
    if (page_referance[PA2PFN(pa)] < 0)
        printf("pa = %p, count = %d\n", pa, page_referance[PA2PFN(pa)]);
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

static int in_freerange = 0;
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  in_freerange = 1;
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
  in_freerange = 0;
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int count;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if (in_freerange == 0)
    dec_page_referance((uint64)pa);

  count = page_referance[PA2PFN((uint64)pa)];
  if (count > 0)
    return;
  else if (count < 0)
    printf("kfree error, pa = %p, count = %d\n", pa, count);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    page_referance[PA2PFN((uint64)r)] = 1;
  }
  return (void*)r;
}
