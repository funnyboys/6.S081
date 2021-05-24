// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct buf buf[NBUF];
struct {
  struct spinlock lock;
  struct buf head;
  uint bucket_ticks;     /* min ticks in current bucket */
} bucket[NBUCKET];

static inline void INIT_LIST_HEAD(struct buf *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct buf *new,
                    struct buf *prev,
                    struct buf *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct buf *new, struct buf *head)
{
    __list_add(new, head, head->next);
}

static inline int list_empty(const struct buf *head)
{
    return head->next == head;
}

static inline void list_remove_node(struct buf *buf)
{
    buf->prev->next = buf->next;
    buf->next->prev = buf->prev;
}

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

static inline int list_nr_entry(struct buf *head)
{
    struct buf *b;
    int nr = 0;
    list_for_each(b, head)
        nr++;
    return nr;
}

#define INDEX(n)    (n % NBUCKET)

void
binit(void)
{
  uint i, index;

  for (i = 0; i < NBUCKET; i++) {
    char name[10] = {0};
    snprintf(name, 10, "bcache%d", i);
    initlock(&bucket[i].lock, name);
    INIT_LIST_HEAD(&bucket[i].head);
    bucket[i].bucket_ticks = 0;
  }

  index = 0;
  for (i = 0; i < NBUF; i++, index++) {
    index = INDEX(index);
    list_add(&buf[i], &bucket[index].head);
    initsleeplock(&buf[i].lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint i, index;

  index = INDEX(blockno);
  acquire(&bucket[index].lock);

  // list empty
  if (list_empty(&bucket[index].head))
    goto steal_buf;

  // Is the block already cached?
  list_for_each(b, &bucket[index].head)
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      if (bucket[index].bucket_ticks < ticks)
        bucket[index].bucket_ticks = ticks;
      release(&bucket[index].lock);
      acquiresleep(&b->lock);
      return b;
    }

  // find free buf in current bucket
  list_for_each(b, &bucket[index].head)
    if (b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bucket[index].lock);
      acquiresleep(&b->lock);
      return b;
    }

  /*
   * 从其它bucket偷取buf时，当前bucket的锁不能释放，否则会导致数据一致性问题
   *    eg:
   *        进程p1和进程p2同时访问一个block b1，但是b1不在bucket对应的链表中
   *        p1释放bucket[index].lock锁
   *        p2查找bucket[index]链表，因为此时bucket[index].lock已经被p1释放
   *        p2和p1一样，发现b1不在链表中
   *        p1和p2都会去recycle一个空闲buf，来缓存b1
   *        block b1在内存中有两份缓存，违背了一致性
   *
   * 参考：
   *        https://github.com/skyzh/xv6-riscv-fall19/issues/10
   *        lab_lock中的描述：
   *            You must maintain the invariant that at most one copy of each block is cached.
   */
steal_buf:
  i = index;
  while (1)
  {
    /*
     * 这里加锁必须是顺序的, 否则会导致死锁
     *      eg: 进程0: acquire(lock0) acquire(lock1)
     *          进程1: acquire(lock1) acquire(lock0)
     */
    i = (i + 1) % NBUCKET;
    if (i == index)
      break;

    acquire(&bucket[i].lock);
    // move from i to index
    if (!list_empty(&bucket[i].head)) {
      list_for_each(b, &bucket[i].head) {
        if (b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          list_remove_node(b);
          list_add(b, &bucket[index].head);

          release(&bucket[i].lock);
          release(&bucket[index].lock);
          acquiresleep(&b->lock);
          return b;
        }
      }
    }
    release(&bucket[i].lock);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  uint i = INDEX(b->blockno);
  if(!holdingsleep(&b->lock))
    panic("brelse");
  releasesleep(&b->lock);
  acquire(&bucket[i].lock);
  b->refcnt--;
  release(&bucket[i].lock);
}

void
bpin(struct buf *b) {
  uint i = INDEX(b->blockno);
  acquire(&bucket[i].lock);
  b->refcnt++;
  release(&bucket[i].lock);
}

void
bunpin(struct buf *b) {
  uint i = INDEX(b->blockno);
  acquire(&bucket[i].lock);
  b->refcnt--;
  release(&bucket[i].lock);
}
