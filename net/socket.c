
#include <netinet/in.h>
#include <sdnet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <logging.h>

void * (*new_conn_fn)(int);

int sd_init(const char *addr, uint16_t port, void *cb(int))
{
    int fd, opt = 1;
    struct sockaddr_in sa;

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
        LOG_ERROR("Unable to create socket fd.\n");
        return -1;
    }
    sa.sin_family = AF_INET;
    if (addr == NULL) {
        sa.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, addr, &sa.sin_addr.s_addr) < 0) {
            LOG_ERROR("inet_pton error,an invalid address specified\n");
            close(fd);
            return -1;
        }
    }
    sa.sin_port = htons(port);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
        LOG_ERROR("Set REUSEADDR error\n");
        close(fd);
        return -1;
    }
    if (bind(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr)) < 0) {
        LOG_ERROR("bind error\n");
        close(fd);
        return -1;
    }
    if (listen(fd, DEFAULT_BACKLOG) < 0) {
        LOG_ERROR("listen error\n");
        close(fd);
    }
    new_conn_fn = cb;
    return fd;
}


int sd_dispatch(int fd)
{
    int connfd;
    for (;;) {
        connfd = accept(fd, NULL, NULL);
        if (connfd < 0) {
            LOG_ERROR("accept error\n");
            continue;
        }
        (void)new_conn_fn(connfd);
    }
    return 0;
}



