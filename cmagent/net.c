/*
 * Copyright (c) 2012 Elite Co., Ltd.
 * Author: Hong Shen <sh@ikwcn.com>
 */

#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/dns.h>
#include <event2/event.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "net.h"
#include "log.h"

#ifndef COMPILE_TARGET_X86
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#define SSL_CERT_PATH "/etc/ca"
static SSL_CTX *ssl_ctx;
#else
#include <logging.h>
#endif

static struct event_base* base;
static struct evdns_base* dns_base = NULL;
static struct event* signal_event;

static void exit_loop(evutil_socket_t fd __attribute__((unused)),
        short events __attribute__((unused)), void* ptr __attribute__((unused))) {
    event_base_loopexit(base, NULL);
}

#ifndef COMPILE_TARGET_X86
// We have to override default certificate verify result, because when system time
// at client side is incorrect, verification fails.
static int override_timeissue_verify(int pre_verify, X509_STORE_CTX* ctx) {
    if (pre_verify == 1) return 1;
    switch (X509_STORE_CTX_get_error(ctx)) {
        case X509_V_ERR_CERT_NOT_YET_VALID:
        case X509_V_ERR_CERT_HAS_EXPIRED:
            // Do we really need these?
        case X509_V_ERR_CRL_NOT_YET_VALID:
        case X509_V_ERR_CRL_HAS_EXPIRED:
            return 1;
        default:
            return 0;
    }
}

// Copy from libevent's manual.
static SSL_CTX *evssl_init() {
    SSL_CTX* server_ctx;

    /* Initialize the OpenSSL library */
    SSL_load_error_strings();
    SSL_library_init();
    if (!RAND_poll()) {
        return NULL;
    }
    server_ctx = SSL_CTX_new(TLSv1_method());
    SSL_CTX_load_verify_locations(server_ctx, NULL, SSL_CERT_PATH);
    SSL_CTX_set_verify(server_ctx, SSL_VERIFY_PEER, &override_timeissue_verify);
    return server_ctx;
}
#endif


#ifndef COMPILE_TARGET_X86
struct event_base* net_init() {
    if ((base = event_base_new()) == NULL) {
        LOG_ERROR("Failed to create new event base.");
        return NULL;
    }
    if ((ssl_ctx = evssl_init()) == NULL) {
        LOG_ERROR("Failed to init ssl.");
        return NULL;
    }
    // Register a signal handler.
    signal_event = evsignal_new(base, SIGINT, exit_loop, NULL);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        LOG_ERROR("Could not create a signal event!");
        return NULL;
    }
    return base;
}
#else
struct event_base* net_init() {
    if ((base = event_base_new()) == NULL) {
        LOG_ERROR("Failed to create new event base.");
        return NULL;
    }
    // Register a signal handler.
    signal_event = evsignal_new(base, SIGINT, exit_loop, NULL);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        LOG_ERROR("Could not create a signal event!");
        return NULL;
    }
    return base;
}
#endif

#ifndef COMPILE_TARGET_X86
void net_free() {
    if (signal_event != NULL) {
        event_del(signal_event);
        event_free(signal_event);
        signal_event = NULL;
    }
    ERR_free_strings();
    if (ssl_ctx != NULL) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
    if (base != NULL) {
        event_base_free(base);
        base = NULL;
    }
}
#else
void net_free() {
    if (signal_event != NULL) {
        event_del(signal_event);
        event_free(signal_event);
        signal_event = NULL;
    }
    if (base != NULL) {
        event_base_free(base);
        base = NULL;
    }
}
#endif


int net_dispatch() {
    return (event_base_dispatch(base) == 0) ? 0 : -1;
}

#ifndef COMPILE_TARGET_X86
struct bufferevent* tcp_connect(void* ptr, char* host, int port,
        bufferevent_data_cb readcb, bufferevent_event_cb eventcb) {
    LOG_DEBUG("Enter tcp_connect(%s, %d)", host, port);
    // Reload DNS configuration before connecting.
    // Cached hosts are always appended, but never removed when calling
    // evdns_base_resolv_conf_parse() in libevent 2.0.21.
    // So we need a newly created dns_base every time.
    if (dns_base != NULL) {
        evdns_base_free(dns_base, 0);
    }
    dns_base = evdns_base_new(base, 1);

    SSL* ssl = SSL_new(ssl_ctx);
    struct bufferevent* bev = bufferevent_openssl_socket_new(base, -1, ssl,
            BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, readcb, NULL, eventcb, ptr);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    if (bufferevent_socket_connect_hostname(bev, dns_base, AF_INET, host, port)
            != 0) {
        bufferevent_free(bev);
        bev = NULL;
    }
    return bev;
}
#else 
struct bufferevent* tcp_connect(const char* host, int port,
        bufferevent_data_cb readcb, bufferevent_event_cb eventcb) {

    struct bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (dns_base != NULL) {
        evdns_base_free(dns_base, 0);
    }
    dns_base = evdns_base_new(base, 1);
    bufferevent_setcb(bev, readcb, NULL, eventcb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    if (bufferevent_socket_connect_hostname(bev, dns_base, AF_INET, host, port)
            != 0) {
        bufferevent_free(bev);
        bev = NULL;
    }
    return bev;
}
#endif

