


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "mm.h"




static struct memblk_list memblk_list; 

static void *__malloc(int size)
{
    return malloc(size);
}


int init_memblk_list(int blk_size, int list_size)
{
    struct memblk_list * ml = &memblk_list;
    struct list_head * l;
    void *buf;
    int i;

    assert(blk_size > 0 && list_size > 0);

    ml->blk_size = blk_size;
    ml->idle = list_size;
    spin_lock_init(&ml->lock);
    INIT_LIST_HEAD(&ml->idle_list);
    INIT_LIST_HEAD(&ml->active_list);
    buf = __malloc((blk_size + (sizeof (struct list_head))) * list_size);
    if (!buf) {
        errno = ENOMEM;
        return -1;
    }
    for (i = 0; i < list_size; i++) {
        l = (struct list_head *)((char*)buf + (i * (blk_size + (sizeof (struct list_head)))));
        INIT_LIST_HEAD(l);
        list_add(l, &ml->idle_list);
    }
    return 0;
}


void *get_mem(int size)
{
    void *p = NULL;
    struct list_head *l;
    struct memblk_list *ml = &memblk_list;
    if (size != ml->blk_size) {
        return NULL;
    }
    spin_lock(&ml->lock);
    if (ml->idle > 0) {
        l = ml->idle_list.next;
        list_move(l, &ml->active_list);
        ml->idle--;
        p = (char *)l + sizeof (struct list_head);
    }
    spin_unlock(&ml->lock);
    return p;
}


/* FIXME: We should have checked if the address of ptr aligned correctly. */
void free_mem(void *ptr)
{
    struct list_head *l;
    struct memblk_list *ml = &memblk_list;

    spin_lock(&ml->lock);
    l = (struct list_head *)((char*)ptr - sizeof (struct list_head));
    list_move(l, &ml->idle_list);
    ml->idle++;
    spin_unlock(&ml->lock);
}

