#ifndef __SDINET_H__
#define __SDINET_H__
#include <stdint.h>

typedef struct sdsocket {
    int fd;
} sdsocket_t;

#define DEFAULT_BACKLOG 80
extern int sd_init(const char *, uint16_t, void *cb(int));

#endif
