#ifndef _LOCK_H_
#define _LOCK_H_
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define spin_lock_init(l) pthread_spin_init(l, PTHREAD_PROCESS_PRIVATE)
#define spin_lock(l) pthread_spin_lock(l)
#define spin_unlock(l) pthread_spin_unlock(l)
#define mutex_lock(l) pthread_mutex_lock(l)
#define mutex_unlock(l) pthread_mutex_unlock(l)

#endif

