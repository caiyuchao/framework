
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <logging.h>
#include <sys/epoll.h>
#include <sdnet.h>
#define MAX_EVENTS 10

void * worker(int fd)
{
    int epollfd, nr_fd, i, n;
    struct epoll_event ev, evs[MAX_EVENTS];
    char buf[1024];
    LOG_INFO("New connection established");
    epollfd = epoll_create(10);
    if (epollfd < 0) {
        LOG_ERROR("epoll_create error\n");
        return NULL;
    }
    ev.events = EPOLLIN ;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
        LOG_ERROR("epoll_ctl error\n");
        return NULL;
    }
    for (;;) {
        nr_fd = epoll_wait(epollfd, evs, MAX_EVENTS, -1);
        if (nr_fd == -1) {
            LOG_ERROR("epoll_wait error\n");
        }
        for (i = 0; i < nr_fd; i++) {
            if (evs[i].data.fd == fd) {
                if (evs[i].events & EPOLLIN) {
                    if ((n = read(fd, buf, sizeof buf)) < 0) {
                        LOG_ERRNO(errno);
                        return NULL;
                    } else if (n == 0) {
                        LOG_INFO("Peer closed connection\n");
                        return NULL;
                    } else {
                        LOG_INFO("%d bytes received", n);
                        buf[n] = 0;
                        LOG_INFO("%s", buf + 4);
                        continue;
                        ev.events |= EPOLLOUT;
                        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                            LOG_ERROR("Unable to add EPOLLOUT\n");
                            return NULL;
                        }
                    }

                } else if (evs[i].events & EPOLLERR) {
                    LOG_ERROR("EPOLLERR\n");
                    return NULL;
                } else if (evs[i].events & EPOLLPRI) {
                    LOG_ERROR("EPOLLPRI\n");
                    return NULL;
                } else if (evs[i].events & EPOLLOUT) {
                    write(fd, buf, strlen(buf));
                    ev.events &= ~EPOLLOUT;
                    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                        LOG_ERROR("Unable to delete EPOLLOUT\n");
                        return NULL;
                    }
                } else {
                    LOG_ERROR("OTHER\n");
                    return NULL;
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int fd;
    log_init(NULL);
    fd = sd_init(NULL, 9999, worker);
    if (fd < 0) {
        LOG_ERROR("error\n");
        return -1;
    }
    sd_dispatch(fd);
    return 0;
    //return cmagent(argc, argv);
}
