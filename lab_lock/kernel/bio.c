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

uint min_ticks_index;
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
//#define INDEX(n)    (0)

#define DEBUG_PRINT

//#define BIG_LOCK

struct spinlock bio_lock;

static void print_list(struct buf *head)
{
#ifdef DEBUG_PRINT
    struct buf *b;
    printf("list %p\n", head);
    list_for_each(b, head)
        printf("%p(%d %d) -> ", b, b->dev, b->blockno);
    printf("\n");
#else
    (void)head;
#endif
}


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
  min_ticks_index = 0xffffffff;

  initlock(&bio_lock, "bio_lock");


  index = 0;
  for (i = 0; i < NBUF; i++, index++) {
    index = INDEX(index);
    //printf("start i = %d, index = %d\n", i, index);
    list_add(&buf[i], &bucket[index].head);
    
    //printf("end i = %d, index = %d\n", i, index);
    initsleeplock(&buf[i].lock, "buffer");
  }

#ifdef DEBUG_PRINT
  for (i = 0; i < NBUCKET; i++)
    printf("bucket[%d] nr %d\n", i, list_nr_entry(&bucket[i].head));
#endif
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  int found = 0;
  struct buf *b;
  uint i, index, min_ticks;
  uint small, big;

  (void)min_ticks;

#ifdef BIG_LOCK
  acquire(&bio_lock);
#endif

  index = INDEX(blockno);
  acquire(&bucket[index].lock);

  // list empty
  if (list_empty(&bucket[index].head))
    goto alloc_bcache;

  // Is the block already cached?
  list_for_each(b, &bucket[index].head) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      if (bucket[index].bucket_ticks < ticks)
        bucket[index].bucket_ticks = ticks;
      release(&bucket[index].lock);

#ifdef DEBUG_PRINT
      printf("[%s %d] dev = %d, index = %d, blockno = %d, b = %p\n", __func__, __LINE__, dev, index, blockno, b);
#endif

#ifdef BIG_LOCK
      release(&bio_lock);
#endif

      acquiresleep(&b->lock);

      return b;
    }
  }

  //printf("[%s %d]\n", __func__, __LINE__);
alloc_bcache:
  release(&bucket[index].lock);

#if 0
  // not found valid buffer in current bucket
  for (i = 0; i < NBUCKET; i++) {
    //if (i == index)
    //  continue;
    acquire(&bucket[i].lock);
    if (!list_empty(&bucket[i].head)) {
      list_for_each(b, &bucket[i].head) {
        if (b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          found = 1;
          print_list(&bucket[i].head);
          list_remove_node(b);
          print_list(&bucket[i].head);

#ifdef DEBUG_PRINT
          printf("[%s %d] dev = %d, i = %d, blockno = %d, b = %p\n", __func__, __LINE__, dev, i, blockno, b);
#endif

          break;
        }
      }
    }
    release(&bucket[i].lock);
    if (found)
      break;
  }

  if (found) {
    index = INDEX(blockno);
    acquire(&bucket[index].lock);
    print_list(&bucket[index].head);

    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    list_add(b, &bucket[index].head);
    print_list(&bucket[index].head);
    bucket[index].bucket_ticks = ticks;
    release(&bucket[index].lock);

#ifdef BIG_LOCK
    release(&bio_lock);
#endif

    acquiresleep(&b->lock);

#ifdef DEBUG_PRINT
    printf("[%s %d] dev = %d, index = %d, blockno = %d, b = %p\n", __func__, __LINE__, dev, index, blockno, b);
#endif
    return b;

  } else
    panic("bget: no buffers");
#else
  // steal buf from other bucket
  for (i = 0; i < NBUCKET; i++) {
    if (i == index)
      continue;
    big = i > index ? i : index;
    small = i + index - big;

    acquire(&bucket[small].lock);
    acquire(&bucket[big].lock);

    // move from i to index
    if (!list_empty(&bucket[i].head)) {
      list_for_each(b, &bucket[i].head) {
        if (b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          found = 1;
          list_remove_node(b);
          list_add(b, &bucket[index].head);
          release(&bucket[big].lock);
          release(&bucket[small].lock);
          acquiresleep(&b->lock);
          return b;
        }
      }
    }

    release(&bucket[big].lock);
    release(&bucket[small].lock);

  }

  (void)found;
  panic("bget: no buffers");
  
  print_list(&bucket[index].head);
#endif
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);

  // valid = 0, bcache invalid, read from disk
  if(!b->valid) {
    if (!holdingsleep(&b->lock))
        panic("brea");
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }

#ifdef DEBUG_PRINT
  if (b->disk == 0)
      printf("[%s %d] b = %p, blockno = %d\n", __func__, __LINE__, b, blockno);
#endif
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

#ifdef DEBUG_PRINT
  printf("[%s %d] b = %p, blockno = %d\n", __func__, __LINE__, b, b->blockno);
#endif
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  acquire(&bucket[i].lock);
  b->refcnt--;

#if 0
  if (b->refcnt == 0) {
    bucket[i].bucket_ticks = ticks;
    // no one is waiting for it.
    //list_remove_node(b);
  }
#endif
  
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
