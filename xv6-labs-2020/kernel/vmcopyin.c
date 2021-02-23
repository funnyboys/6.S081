#include "param.h"
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"

//
// This file contains copyin_new() and copyinstr_new(), the
// replacements for copyin and coyinstr in vm.c.
//

static struct stats {
  int ncopyin;
  int ncopyinstr;
} stats;

int
statscopyin(char *buf, int sz) {
  int n;
  n = snprintf(buf, sz, "copyin: %d\n", stats.ncopyin);
  n += snprintf(buf+n, sz, "copyinstr: %d\n", stats.ncopyinstr);
  return n;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
    uint64 pa;
    pte_t *pte;
    struct proc *p = myproc();

    if (srcva >= p->sz || srcva+len >= p->sz || srcva+len < srcva)
        return -1;

    pte = walk(pagetable, srcva, 0);
    pa = PTE2PA(*pte);

    printf("[%s %d] pa is %p\n", __func__, __LINE__, pa);
    printf("[%s %d] r_satp is %p\n", __func__, __LINE__, r_satp());
    printf("[%s %d] pagetable is %p\n", __func__, __LINE__, pagetable);
    printf("[%s %d] p->pagetable is %p\n", __func__, __LINE__, MAKE_SATP(p->pagetable));
    printf("[%s %d] p->k_pagetable is %p\n", __func__, __LINE__, MAKE_SATP(p->k_pagetable));
    printf("[%s %d] dst is %p, srcva is %p\n", __func__, __LINE__, dst, srcva);
    vmprint(p->k_pagetable);
    pte = walk(p->k_pagetable, srcva, 0);
    if (pte == 0)
        printf("[%s %d] r_satp is %p\n", __func__, __LINE__, r_satp());
    if ((*pte & PTE_V) == 0)
        printf("[%s %d] r_satp is %p\n", __func__, __LINE__, r_satp());
    if ((*pte & PTE_U) == 0)
        printf("[%s %d] r_satp is %p\n", __func__, __LINE__, r_satp());

    pte = walk(p->k_pagetable, (uint64)dst, 0);
    if (pte == 0)
        printf("[%s %d] r_satp is %p\n", __func__, __LINE__, r_satp());
    if ((*pte & PTE_V) == 0)
        printf("[%s %d] r_satp is %p\n", __func__, __LINE__, r_satp());
    if ((*pte & PTE_U) == 0)
        printf("[%s %d] r_satp is %p\n", __func__, __LINE__, r_satp());

    memmove((void *) dst, (void *)srcva, len);
    stats.ncopyin++;   // XXX lock
    return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  struct proc *p = myproc();
  char *s = (char *) srcva;
  
  stats.ncopyinstr++;   // XXX lock
  for(int i = 0; i < max && srcva + i < p->sz; i++){
    dst[i] = s[i];
    if(s[i] == '\0')
      return 0;
  }
  return -1;
}
