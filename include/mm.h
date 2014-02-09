
#ifndef _MM_H_
#define _MM_H_

#include "list.h"
#include "../lib/lock.h"

struct memblk_list {
    int blk_size;
    int idle;
    pthread_spinlock_t lock;
    struct list_head idle_list;
    struct list_head active_list;
};

struct mempool {
    int type;
    pthread_mutex_t lock;
    struct memblk_list **memblk;
};

extern int init_memblk_list(int blk_size, int list_size);
extern void free_mem(void *ptr);
extern void *get_mem(int size);

#endif

