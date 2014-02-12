/*
 * Copyright (c) 2012 Elite Co., Ltd.
 * Author: Hong Shen <sh@ikwcn.com>
 */

#ifndef NET_H_
#define NET_H_

#include <event2/bufferevent.h>

/**
 * Initialize data which is needed at the beginning.
 *
 * <p>This function must be called before all behaviors on network.
 *
 * @return the created {@code event_base} or NULL
 */
struct event_base* net_init();

void net_free();

/**
 * Enter the event dispatching loop.
 *
 * @return 0 on success, -1 on failure
 */
int net_dispatch();

/**
 * Connect to a host through TCP protocol.
 *
 * @param ptr pointer to be passed to the callbacks
 * @param host the destination host name to connect
 * @param port the destination port to connect
 * @param read_callback the callback function when received data from that host
 * @param event_callback the callback function when received events
 * @return the {@code bufferevent} of created TCP connection, NULL on failure
 */
#ifndef COMPILE_TARGET_X86
struct bufferevent* tcp_connect(void* ptr, char* host, int port,
    bufferevent_data_cb read_callback, bufferevent_event_cb event_callback);
#else
struct bufferevent* tcp_connect(const char* host, int port,
    bufferevent_data_cb read_callback, bufferevent_event_cb event_callback);
#endif

#endif /* NET_H_ */
