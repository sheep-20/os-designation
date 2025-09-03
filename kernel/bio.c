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

#define NBUCKETS 13
#define HASHFN(dev, blockno) (((dev) * 13 + (blockno)) % NBUCKETS)

struct bucket {
  struct spinlock lock;
  struct buf head;    // LRU list head
};

struct {
  struct buf buf[NBUF];
  struct bucket buckets[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  // Initialize bucket locks and LRU lists
  for (int i = 0; i < NBUCKETS; i++) {
    initlock(&bcache.buckets[i].lock, "bcache.bucket");
    bcache.buckets[i].head.next = &bcache.buckets[i].head;
    bcache.buckets[i].head.prev = &bcache.buckets[i].head;
  }

  // Distribute buffers across buckets evenly
  for (int i = 0; i < NBUF; i++) {
    b = &bcache.buf[i];
    initsleeplock(&b->lock, "buffer");
    b->dev = 0;
    b->blockno = 0;
    b->valid = 0;
    b->refcnt = 0;

    // Distribute buffers evenly across buckets
    int h = i % NBUCKETS;
    struct bucket *bucket = &bcache.buckets[h];
    b->next = bucket->head.next;
    b->prev = &bucket->head;
    bucket->head.next->prev = b;
    bucket->head.next = b;
  }
}

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int h = HASHFN(dev, blockno);
  struct bucket *bucket = &bcache.buckets[h];

  // Check if block is already cached in target bucket
  acquire(&bucket->lock);
  for (b = bucket->head.next; b != &bucket->head; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Look for unused buffer in target bucket first (from LRU end)
  for (b = bucket->head.prev; b != &bucket->head; b = b->prev) {
    if (b->refcnt == 0) {
      // Reuse this buffer
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      // Move to MRU position
      b->prev->next = b->next;
      b->next->prev = b->prev;
      b->next = bucket->head.next;
      b->prev = &bucket->head;
      bucket->head.next->prev = b;
      bucket->head.next = b;

      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bucket->lock);

  // Target bucket has no free buffers, scan other buckets
  for (int i = 0; i < NBUCKETS; i++) {
    if (i == h) continue; // already checked target bucket

    struct bucket *other = &bcache.buckets[i];
    acquire(&other->lock);
    
    // Look for unused buffer in this bucket
    for (b = other->head.prev; b != &other->head; b = b->prev) {
      if (b->refcnt == 0) {
        // Remove from old bucket
        b->prev->next = b->next;
        b->next->prev = b->prev;
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release(&other->lock);

        // Add to target bucket
        acquire(&bucket->lock);
        b->next = bucket->head.next;
        b->prev = &bucket->head;
        bucket->head.next->prev = b;
        bucket->head.next = b;
        release(&bucket->lock);

        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&other->lock);
  }

  panic("bget: no buffers");
}

struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b = bget(dev, blockno);
  if (!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

void
bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

void
brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int h = HASHFN(b->dev, b->blockno);
  struct bucket *bucket = &bcache.buckets[h];

  acquire(&bucket->lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // Only move to MRU if it's not already there
    if (bucket->head.next != b) {
      // Remove from current position
      b->next->prev = b->prev;
      b->prev->next = b->next;
      // Insert at MRU position
      b->next = bucket->head.next;
      b->prev = &bucket->head;
      bucket->head.next->prev = b;
      bucket->head.next = b;
    }
  }
  release(&bucket->lock);
}

void
bpin(struct buf *b)
{
  int h = HASHFN(b->dev, b->blockno);
  struct bucket *bucket = &bcache.buckets[h];
  acquire(&bucket->lock);
  b->refcnt++;
  release(&bucket->lock);
}

void
bunpin(struct buf *b)
{
  int h = HASHFN(b->dev, b->blockno);
  struct bucket *bucket = &bcache.buckets[h];
  acquire(&bucket->lock);
  b->refcnt--;
  release(&bucket->lock);
}

