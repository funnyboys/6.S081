struct buf {
  int valid;   /* has data been read from disk?
                * valid = 0, buf中的缓存无效，需从disk读
                * valid = 1, buf中的缓存有效，可直接用
                */
  int disk;    /* does disk "own" buf?
                * disk = 1，disk正在处理buf
                * disk = 0, disk处理完毕buf
                */
  uint dev;
  uint blockno;
  struct sleeplock lock;    // protect buf->data
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

