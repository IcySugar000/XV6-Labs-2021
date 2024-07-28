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

#define MAX_BUCKETS 13

struct bucket {
  struct spinlock lock;
  struct buf head;
};

struct {
  struct buf buf[NBUF];
  struct bucket buckets[MAX_BUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  char name[20];
  for(int i=0; i<MAX_BUCKETS; ++i) {
    snprintf(name, sizeof(name), "bcache_%d", i);
    initlock(&bcache.buckets[i].lock, name);

    bcache.buckets[i].head.prev = &bcache.buckets[i].head;
    bcache.buckets[i].head.next = &bcache.buckets[i].head;
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.buckets[0].head.next;
    b->prev = &bcache.buckets[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[0].head.next->prev = b;
    bcache.buckets[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = blockno % MAX_BUCKETS;

  acquire(&bcache.buckets[id].lock);

  // Is the block already cached?
  for(b = bcache.buckets[id].head.next; b != &bcache.buckets[id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;

      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);

      release(&bcache.buckets[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  struct buf *tmp, *lru = 0;
  
  for(int i=0; i<MAX_BUCKETS; ++i) {
    int cur_bucket = (i+id)%MAX_BUCKETS;
    if(cur_bucket != id) {
      if(!holding(&bcache.buckets[cur_bucket].lock)) acquire(&bcache.buckets[cur_bucket].lock);
      else continue;
    }

    for(tmp=bcache.buckets[cur_bucket].head.next; tmp!=&bcache.buckets[cur_bucket].head; tmp = tmp->next) {
      if(tmp->refcnt==0 && (lru==0 || tmp->timestamp < lru->timestamp)) lru = tmp;
    }

    if(lru) {
      // 头插法
      if(cur_bucket != id) {
        lru->next->prev = lru->prev;
        lru->prev->next = lru->next;
        release(&bcache.buckets[cur_bucket].lock);

        lru->next = bcache.buckets[id].head.next;
        lru->prev = &bcache.buckets[id].head;
        bcache.buckets[id].head.next->prev = lru;
        bcache.buckets[id].head.next = lru;
      }

      lru->dev = dev;
      lru->blockno = blockno;
      lru->valid = 0;
      lru->refcnt = 1;

      acquire(&tickslock);
      lru->timestamp = ticks;
      release(&tickslock);

      release(&bcache.buckets[id].lock);
      acquiresleep(&lru->lock);
      return lru;
    }
    else {
      if(cur_bucket != id) release(&bcache.buckets[cur_bucket].lock);
    }
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
  if(!holdingsleep(&b->lock))
    panic("brelse");

  int id = b->blockno % MAX_BUCKETS;

  releasesleep(&b->lock);

  acquire(&bcache.buckets[id].lock);
  b->refcnt--;

  acquire(&tickslock);
  b->timestamp = ticks;
  release(&tickslock);
  
  release(&bcache.buckets[id].lock);
}

void
bpin(struct buf *b) {
  int id = b->blockno % MAX_BUCKETS;
  acquire(&bcache.buckets[id].lock);
  b->refcnt++;
  release(&bcache.buckets[id].lock);
}

void
bunpin(struct buf *b) {
  int id = b->blockno % MAX_BUCKETS;
  acquire(&bcache.buckets[id].lock);
  b->refcnt--;
  release(&bcache.buckets[id].lock);
}


