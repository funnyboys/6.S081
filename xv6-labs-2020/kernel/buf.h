struct buf {
  int valid;   /* has data been read from disk?
                * valid = 0, buf�еĻ�����Ч�����disk��
                * valid = 1, buf�еĻ�����Ч����ֱ����
                */
  int disk;    /* does disk "own" buf?
                * disk = 1��disk���ڴ���buf
                * disk = 0, disk�������buf
                */
  uint dev;
  uint blockno;
  struct sleeplock lock;    // protect buf->data
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

