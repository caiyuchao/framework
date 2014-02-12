#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <event2/bufferevent.h>

#include <sdnet.h>

#include <logging.h>
#include <stdlib.h>

#include <event2/event.h>
#include <sys/socket.h>
#include <sys/types.h>

void input_event_cb(struct bufferevent *bev, short ev_type, void *arg)
{
    if (ev_type & BEV_EVENT_ERROR) {
        LOG_ERROR("input error\n");
        exit(1);
    } else if (ev_type & BEV_EVENT_READING) {
        LOG_INFO("bev reading\n");
    }

}


void input_data_cb(struct bufferevent *bev, void *ctx)
{
    char buf[1024];
    struct bufferevent *net = ctx;
    struct evbuffer *in = NULL;
    size_t len;
    in = bufferevent_get_input(bev);
    len = evbuffer_get_length(in);
    if (len) {
        evbuffer_remove(in, buf, len);
        bufferevent_write(net, buf, len);
    }
}


void net_event_cb(struct bufferevent *bev, short events, void *ptr)
{
    struct event_base *base = ptr;
    struct bufferevent *input = NULL;
    int flags = -1;
    if (events & BEV_EVENT_CONNECTED) {
        flags = fcntl(STDIN_FILENO, F_GETFL);
        if (flags < 0) {
            LOG_ERROR("Failed to get stdin flags\n");
            LOG_ERRNO(errno);
            exit(1);
        }
        if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) < 0) {
            LOG_ERROR("Failed to set stdin with NONBLOCK\n");
            exit(1);
        }
        input = bufferevent_socket_new(base, STDIN_FILENO, BEV_OPT_DEFER_CALLBACKS);
        if (input == NULL) {
            LOG_ERROR("Failed to new an bufferevent\n");
            exit(1);
        }
        bufferevent_setcb(input, input_data_cb, input_data_cb, input_event_cb, bev);
        LOG_INFO("Connection has been established, ready for input:\n");
        bufferevent_enable(input, EV_READ);
    } else if (events & BEV_EVENT_ERROR) {
        LOG_ERROR("Connect error\n");
        exit(1);
    } else if (events & BEV_EVENT_EOF) {
        LOG_ERROR("The other side closed connection\n");
        exit(1);
    } else {
        LOG_ERROR("UNKNOW\n");
        exit(1);

    }
}


void net_data_cb(struct bufferevent *bev, void *ptr)
{
    char buf[1024];
    size_t len;
    struct evbuffer *in = bufferevent_get_input(bev);
    len = evbuffer_get_length(in);
    if (len) {
        evbuffer_remove(in, buf, len);
        buf[len] = 0;
        LOG_INFO("%s\n", buf);
    }

}


extern int cmagent(int , char **);


int main(int argc, char **argv)
{
    struct event_base * base = NULL;
    struct bufferevent * bev = NULL, *input = NULL;
    struct sockaddr_in sa;
    int flags = -1;
    char buf[128] = "hello";
    log_init(NULL);
    return cmagent(argc, argv);
    base = event_base_new();
    if (base == NULL) {
        LOG_ERROR("event_base_new() error\n");
        return -1;
    }
    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (bev == NULL) {
        LOG_ERROR("event_base_new() error\n");
        return -1;
    }
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(0x7f000001);
    sa.sin_port = htons(9999);
    bufferevent_setcb(bev, net_data_cb, NULL, net_event_cb, base);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    if (bufferevent_socket_connect(bev, (struct sockaddr *)&sa, sizeof sa) < 0) {
        bufferevent_free(bev);
        LOG_ERROR("connect error\n");
        return -1;
    }
    event_base_dispatch(base);
    return 0;
}
