#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "spinlock.h"
#include "proc.h"

/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable;

extern char etext[];  // kernel.ld sets this to end of kernel code.

extern char trampoline[]; // trampoline.S

/*
 * create a direct-map page table for the kernel.
 */
void kvminit(pagetable_t pagetable)
{
  // uart registers
  mappages(pagetable, UART0, PGSIZE, UART0, PTE_R | PTE_W);

  // virtio mmio disk interface
  mappages(pagetable, VIRTIO0, PGSIZE, VIRTIO0, PTE_R | PTE_W);

  /* 
   * CLINT 相关地址在两个地方使用: timerinit() 和 timervec()
   *    start() -> timerinit(): 初始化 timer 中断(在machine mode初始化)
   *    timervec(): timer 中断处理程序(在machine mode下处理)
   * 而 machine mode 不支持 paging, 直接访问物理地址
   * 所以内核中不需要对 CLINT 进行映射
   * 因此内核虚拟地址的低地址为 PLIC 而不是 CLINT
   */
#if 0
  // CLINT
  mappages(pagetable, CLINT, 0x10000, CLINT, PTE_R | PTE_W);
#endif

  // PLIC
  mappages(pagetable, PLIC, 0x400000, PLIC, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  mappages(pagetable, KERNBASE, (uint64)etext-KERNBASE, KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  mappages(pagetable, (uint64)etext, PHYSTOP-(uint64)etext, (uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64)trampoline, PTE_R | PTE_X);
}

void kvmuninit(pagetable_t pagetable)
{
  uvmunmap(pagetable, UART0, PGSIZE/PGSIZE, 0);

  uvmunmap(pagetable, VIRTIO0, PGSIZE/PGSIZE, 0);

  /* 参考 kvminit() 中的注释 */
#if 0
  uvmunmap(pagetable, CLINT, 0x10000/PGSIZE, 0);
#endif

  uvmunmap(pagetable, PLIC, 0x400000/PGSIZE, 0);

  uvmunmap(pagetable, KERNBASE, ((uint64)etext-KERNBASE)/PGSIZE, 0);

  uvmunmap(pagetable, (uint64)etext, (PHYSTOP-(uint64)etext)/PGSIZE, 0);

  uvmunmap(pagetable, TRAMPOLINE, PGSIZE/PGSIZE, 0);
}


// Switch h/w page table register to the kernel's page table,
// and enable paging.
void
kvminithart(pagetable_t pagetable)
{
  w_satp(MAKE_SATP(pagetable));
  sfence_vma();
}

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if(va >= MAXVA)
    panic("walk");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pagetable[PX(level, va)];
    if(*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if(!alloc || (pagetable = (pde_t*)kalloc()) == 0)
        return 0;
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  return &pagetable[PX(0, va)];
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;

  if(va >= MAXVA)
    return 0;

  pte = walk(pagetable, va, 0);
  if(pte == 0)
    return 0;
  if((*pte & PTE_V) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  pa = PTE2PA(*pte);
  return pa;
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void
kvmmap(uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kernel_pagetable, va, sz, pa, perm) != 0)
    panic("kvmmap");
}

// use current kernel pagetable 
// translate a kernel virtual address to
// a physical address.
// only needed for addresses on the stack.
// assumes va is page aligned.
uint64
kvmpa(uint64 va)
{
  uint64 off = va % PGSIZE;
  pte_t *pte;
  uint64 pa;

  pte = walk(myproc()->k_pagetable, va, 0);
  if(pte == 0)
    panic("kvmpa");
  if((*pte & PTE_V) == 0)
    panic("kvmpa");
  pa = PTE2PA(*pte);
  return pa+off;
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int
mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  uint64 a, last;
  pte_t *pte;

  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for(;;){
    if((pte = walk(pagetable, a, 1)) == 0)
      return -1;
    if(*pte & PTE_V) {
      printf("pagetable is %p, pa is %p, va is %p\n", pagetable, pa, a);
      panic("remap");
    }
    *pte = PA2PTE(pa) | perm | PTE_V;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      panic("uvmunmap: walk");
    if((*pte & PTE_V) == 0)
      panic("uvmunmap: not mapped");
    if(PTE_FLAGS(*pte) == PTE_V)
      panic("uvmunmap: not a leaf");
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
    }
    *pte = 0;
  }
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t
uvmcreate()
{
  pagetable_t pagetable;
  pagetable = (pagetable_t) kalloc();
  if(pagetable == 0)
    return 0;
  memset(pagetable, 0, PGSIZE);
  return pagetable;
}

// Load the user initcode into address 0 of pagetable,
// for the very first process.
// sz must be less than a page.
void
uvminit(pagetable_t pagetable, uchar *src, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W|PTE_R|PTE_X|PTE_U);
  memmove(mem, src, sz);
}

// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uint64
uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  char *mem;
  uint64 a;

  if(newsz < oldsz)
    return oldsz;

  oldsz = PGROUNDUP(oldsz);
  for(a = oldsz; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pagetable, a, PGSIZE, (uint64)mem, PTE_W|PTE_X|PTE_R|PTE_U) != 0){
      kfree(mem);
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64
uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  if(newsz >= oldsz)
    return oldsz;

  if(PGROUNDUP(newsz) < PGROUNDUP(oldsz)){
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);
  }

  return newsz;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if(pte & PTE_V){
      panic("freewalk: leaf");
    }
  }
  kfree((void*)pagetable);
}

// Free user memory pages,
// then free page-table pages.
void
uvmfree(pagetable_t pagetable, uint64 sz)
{
  if(sz > 0)
    uvmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
  freewalk(pagetable);
}

// Given a parent process's page table, copy
// its memory into a child's page table.
// Copies both the page table and the
// physical memory.
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  char *mem;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto err;
    memmove(mem, (char*)pa, PGSIZE);
    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
      kfree(mem);
      goto err;
    }
  }
  return 0;

 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void
uvmclear(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  
  pte = walk(pagetable, va, 0);
  if(pte == 0)
    panic("uvmclear");
  *pte &= ~PTE_U;
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
#if 0
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
#else
  return copyin_new(pagetable, dst, srcva, len);
#endif
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
#if 0
  uint64 n, va0, pa0;
  int got_null = 0;

  while(got_null == 0 && max > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > max)
      n = max;

    char *p = (char *) (pa0 + (srcva - va0));
    while(n > 0){
      if(*p == '\0'){
        *dst = '\0';
        got_null = 1;
        break;
      } else {
        *dst = *p;
      }
      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PGSIZE;
  }
  if(got_null){
    return 0;
  } else {
    return -1;
  }
#else
  return copyinstr_new(pagetable, dst, srcva, max);
#endif
}

/* 这里 %p 类似 0x%x 直接打印十六进制 */
static void print_pagetable(pagetable_t pagetable, int *depth)
{
    int i;
    char *prefix[4] = {"..", ".. ..", ".. .. ..", ".. .. .. .."};
    for (i = 0; i < MAX_PTE_NR; i++) {
        pte_t *pte = &pagetable[i];
        if (pte == 0)
            continue;
        if ((*pte & PTE_V) == 0)
            continue;

        printf("%s", prefix[*depth]);
        if ((*pte & (PTE_R|PTE_W|PTE_X)) == 0) {
            // this PTE points to a lower-level page table.
            printf("%d: pte %p pa %p\n", i, *pte, PTE2PA(*pte));
            *depth = *depth + 1;
            print_pagetable((pagetable_t)(PTE2PA(*pte)), depth);
        } else
            printf("%d: pte %p pa %p\n", i, *pte, PTE2PA(*pte));

    }
    *depth = *depth - 1;
}

void vmprint(pagetable_t pagetable)
{
    int depth = 0;
    printf("page table %p\n", pagetable);
    print_pagetable(pagetable, &depth);
}

/*
 * 将旧页表中的地址映射到内核页表中
 * 内核页表中虚拟地址与用户态页表下使用的相同
 * 只映射地址，不再单独分配内存
 */
int map_uva_in_kpgt(pagetable_t old_pagetable, pagetable_t kernel_pagetable, uint64 va, uint64 npages)
{
    int flags;
    uint64 i, pa;
    pte_t *pte;

    if ((va % PGSIZE) != 0) {
        printf("uvmunmap: va is not aligned, va = %p\n", va);
        return 1;
    }

    for (i = 0; i < npages; i++, va += PGSIZE) {
        if (va >= MAX_UVA_KERNEL)
            break;
        pte = walk(old_pagetable, va, 0);
        if (pte == 0)
            continue;
        if ((*pte & PTE_V) == 0)
            continue;
        pa = PTE2PA(*pte);
        flags = PTE_FLAGS(*pte);
        flags &= (~PTE_U);
        if (mappages(kernel_pagetable, va, PGSIZE, pa, flags) != 0) {
            printf("[%s %d] va is %p, pa is %p\n", __func__, __LINE__, va, pa);
            return 1;
        }
    }

    return 0;
}

/*
 * unmap 内核页表中的用户态地址映射信息
 * 自动跳过未映射的虚拟地址
 */
void unmap_uva_in_kpgt(pagetable_t kernel_pagetable, uint64 va, uint64 npages)
{
    uint64 i;

    if ((va % PGSIZE) != 0) {
        printf("unmap_uva_in_kpgt: va is not aligned, va = %p\n", va);
        return;
    }

    for (i = 0; i < npages; i++, va += PGSIZE) {
        if (va >= MAX_UVA_KERNEL)
            break;
        pte_t *pte = walk(kernel_pagetable, va, 0);
        if (pte == 0)
            continue;
        if ((*pte & PTE_V) == 0)
            continue;
        
        uvmunmap(kernel_pagetable, va, 1, 0);
    }
}

/* 
 * 在内核页表中寻找最后一个未使用的用户态映射虚拟地址
 * 地址范围：0 -- MAX_UVA_KERNEL
 */
uint64 find_last_valid_va(pagetable_t kernel_pagetable)
{
    pte_t *pte;
    uint64 va;

    for (va = MAX_UVA_KERNEL - PGSIZE; va >= 0; va -= PGSIZE) {
        pte = walk(kernel_pagetable, va, 0);
        if (pte == 0)
            return va;
        if ((*pte & PTE_V) == 0)
            return va;
    }

    return INVALID_VA;
}

/*
 * 以 old_va 为虚拟地址, 将 PGSIZE 的数据拷贝到 new_va
 *
 * old_va: 待拷贝数据的虚拟地址
 * old_pagetable: old_va 对应的页表
 * new_va: 存放数据的虚拟地址, 需分配内存
 * new_pagetable: new_va 对应的页表
 * current_pagetable: 当前使用的页表
 *
 * 1. 计算得到 old_va 对应的物理地址 old_pa, 为 new_va 分配对应的一个 PAGE
 * 2. 将 old_pa, new_pa 分别映射到 current_pagetable
 * 3. 拷贝数据
 * 4. 取消 current_pagetable 对 old_pa, new_pa 的映射
 * 5. 添加 new_pagetable 对 new_va 和 new_pa 的映射
 */
int copy_data_across_pagetable(uint64 old_va, pagetable_t old_pagetable, 
        uint64 new_va, pagetable_t new_pagetable, pagetable_t current_pagetable, int perm)
{
    pte_t *pte;

    uint64 cur_old_va, cur_new_va;
    uint64 old_pa, new_pa;

    /* step 1 */
    pte = walk(old_pagetable, old_va, 0);
    if (pte == 0) {
        printf("[%s %d]\n", __func__, __LINE__);
        return 1;
    }
    if ((*pte & PTE_V) == 0) {
        printf("[%s %d]\n", __func__, __LINE__);
        return 2;
    }
    old_pa = PTE2PA(*pte);

    new_pa = (uint64)kalloc();
    if (new_pa == 0) {
        printf("[%s %d]\n", __func__, __LINE__);
        return 3;
    }

    /* step 2 */
    if (current_pagetable != old_pagetable) {
        cur_old_va = find_last_valid_va(current_pagetable);
        if (cur_old_va == INVALID_VA) {
            printf("[%s %d]\n", __func__, __LINE__);
            return 4;
        }
        if (mappages(current_pagetable, cur_old_va, PGSIZE, (uint64)old_pa, perm) != 0) {
            printf("[%s %d]\n", __func__, __LINE__);
            return 5;
        }
    } else
        cur_old_va = old_va;

    cur_new_va = find_last_valid_va(current_pagetable);
    if (cur_new_va == INVALID_VA) {
        printf("[%s %d]\n", __func__, __LINE__);
        return 6;
    }
    if (mappages(current_pagetable, cur_new_va, PGSIZE, (uint64)new_pa, perm) != 0) {
        printf("[%s %d]\n", __func__, __LINE__);
        return 7;
    }

    /* step 3 */
    memmove((void *)cur_new_va, (void *)cur_old_va, PGSIZE);

    /* step 4 */
    if (current_pagetable != old_pagetable)
        uvmunmap(current_pagetable, cur_old_va, 1, 0);      //not free
    uvmunmap(current_pagetable, cur_new_va, 1, 0);          //not free

    /* step 5 */
    if ((mappages(new_pagetable, new_va, PGSIZE, new_pa, perm) != 0)) {
        printf("[%s %d]\n", __func__, __LINE__);
        uvmunmap(new_pagetable, new_va, 1, 0);      //not free
        return 8;
    }

    return 0;
}
