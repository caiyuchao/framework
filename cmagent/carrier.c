/*
 * Copyright (c) 2012 Elite Co., Ltd.
 * Author: Hong Shen <sh@ikwcn.com>
 */

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/dns.h>
#include <event2/event.h>
#include <event2/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <jansson.h>
#include "carrier.h"

#include "frame_decoder.h"
#include "log.h"
#include "net.h"
#ifndef COMPILE_TARGET_X86
#include <libtw.h>
#include <openssl/err.h>
#include <auth.h>
#else
#include <cmagent.h>
#endif


static struct bufferevent* bev;
static struct event* reload_event;
static struct event* idle_timer;
static struct event* connect_timer;
static struct event* keepalive_read_timer;
static struct event* keepalive_write_timer;


#ifndef COMPILE_TARGET_X86
struct timeval idle_timeout = {20, 0};
struct timeval connect_timeout = {120, 0};
struct timeval read_timeout = {330, 0};
struct timeval write_timeout = {300, 0};
static int count = 1;
static int c_index = 0;
static char* names[] = { "carrier.hiwifi.com" };
static int port = 443;
static char token[TOKEN_LEN];
static int connected = 0;
#else
struct timeval idle_timeout = {2, 0};
struct timeval connect_timeout = {12, 0};
struct timeval read_timeout = {5, 0};
struct timeval write_timeout = {3, 0};
static int count = 1;
static int c_index = 0;
static char* names[] = { "localhost" };
static int port = 9999;
static int connected = 0;
static void reconnect(void);
void carrier_send(char *);
#endif

static const unsigned int PREFIX_LENGTH = 4;


static void carrier_try_next() {
    // Round-robin
    c_index = (c_index + 1) % count;
    evtimer_add(idle_timer, &idle_timeout);
}

static int shell_execute(char* message) {
    int fd[2];
    if (pipe(fd) < 0) {
        return -1;
    }
    char lua_path[] = "/usr/bin/lua";
    char full_path[] = "/usr/lib/cmagent/executor.lua";
    pid_t pid = fork();
    if (pid < 0) {
        return -2;
    }
    // Created pipe and child process successfully.
    if (pid == 0) {
        // Child process.
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        execl(lua_path, lua_path, full_path, (char *) 0);
        exit(1);
    } else if (pid > 0) {
        // Parent process.
        close(fd[0]);
        LOG_DEBUG("New process pid: %d", pid);
        write(fd[1], message, strlen(message));
        close(fd[1]);
    }
    return 0;
}


static void connect_timeout_cb(void)
{
    agent_ctx_t *c = get_agent_ctx();
    if (c->conn_state != AGENT_CONNECTING && c->conn_state != AGENT_UNKNOW) { 
        LOG_ERROR("A fetal error detected, connection state untracable");
        c->conn_state = AGENT_UNKNOW;
    } else {
        c->disconn_reason = SSL_CONNECTION_ERROR;
        c->nr_failure++;
    }
    reconnect();
}


static void keepalive_read_timeout_cb(void)
{
    agent_ctx_t *c = get_agent_ctx();
    if (c->conn_state != AGENT_CONNECTED && c->conn_state != AGENT_UNKNOW) {
        LOG_ERROR("A fetal error detected, connection state untracable");
        c->conn_state = AGENT_UNKNOW;
    } else {
        c->disconn_reason = KEEPALIVE_ERROR;
    }
    reconnect();
}

static int service_dispatch(char* message) {
    json_error_t error;
    json_t* root = json_loads(message, 0, &error);
    int resp = -1;
    if (!root) {
        LOG_ERROR("Malformed JSON: %s", message);
        return -1;
    }
    char* msg_id = NULL;
    resp = json_unpack(root, "{s:s}", "msg_id", &msg_id);
    if (resp != 0) {
        // Heartbeat message "{}" goes here.
        goto err;
    }
    // Use double forks to deal with zombie process.
    pid_t pid = fork();
    if (pid < 0) {
        return -2;
    }
    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else if (pid == 0) {
        shell_execute(message);
        exit(0);
    }
    LOG_INFO("Message %s is dispatched.", msg_id);

    json_t* json = json_pack("{s:s,s:s}", "type", "reply", "msg", msg_id);
    char* buf2 = json_dumps(json, 0);
    carrier_send(buf2);
    free(buf2);
    json_decref(json);
err:
    json_decref(root);
    return resp;
}



static void disconnect() {
    LOG_INFO("disconnect()");
    connected = 0;
    evtimer_del(keepalive_read_timer);
    evtimer_del(keepalive_write_timer);
    if (bev != NULL) {
        bufferevent_free(bev);
        bev = NULL;
    }
}

static void reconnect(void) {
    agent_ctx_t * c = get_agent_ctx();
    struct tm tmp_tm;
    disconnect();
    if (c->conn_state == AGENT_CONNECTED) {
        c->conn_state = AGENT_DISCONNECTED;
    } else if (c->conn_state != AGENT_DISCONNECTED && c->conn_state != AGENT_UNKNOW) {
        LOG_ERROR("A fetal error detected, agent state untracable");
        c->conn_state = AGENT_UNKNOW;
    }
    c->retry_tm = time(NULL) + idle_timeout.tv_sec;
    localtime_r(&c->retry_tm, &tmp_tm);
    LOG_DEBUG("Cmagent is in %s state due to %s, reconnect at %02d:%02d:%02d, tried %d time(s).",
            get_agent_state_string(c->conn_state),
            get_disconn_reason_string(c->disconn_reason),
            tmp_tm.tm_hour, tmp_tm.tm_min, tmp_tm.tm_sec,
            c->nr_failure);
    dump_agent_ctx();
    carrier_try_next();
}

static void call(evutil_socket_t unused1 __attribute__((unused)),
        short unused2 __attribute__((unused)), void* arg) {
    void (*func)() = arg;
    func();
}

#ifndef COMPILE_TARGET_X86
/**
 * Dispatch messages from Carrier to the corresponding service.
 *
 * @param bev the {@code bufferevent} which contains the data from Carrier
 * @param ptr unused
 */
static void on_data(struct bufferevent* bev,
        void* ptr __attribute__ ((unused))) {
    LOG_DEBUG("Data received");
    evtimer_add(keepalive_read_timer, &read_timeout);
    struct evbuffer* input = bufferevent_get_input(bev);
    frame_decoder_read(input, service_dispatch, reconnect);
}
#else
static void on_data(struct bufferevent* bev,
        void* ptr __attribute__ ((unused))) {
    LOG_DEBUG("Data received");
    evtimer_add(keepalive_read_timer, &read_timeout);
}

#endif



/**
 * Sends the dynamic status to server.
 *
 * @return 0 on success, -1 on failure
 */
static int send_status() {
    json_t* json = json_pack("{s:s}", "type", "status");
    char* buf = json_dumps(json, 0);
    carrier_send(buf);
    free(buf);
    json_decref(json);
    return 0;
}

/**
 * Send the static info of Worker to Carrier.
 *
 * @param token the token got from auth server
 * @return 0 on success, -1 on failure
 */
#ifdef COMPILE_TARGET_X86
static int send_info(char* token) {
    json_t* json = json_pack("{s:s,s:s,s:s,s:s}",
            "type", "info",
            "model", "fake",
            "mac", "ff:ff:ff:11:11:11",
            "token", "token");
    char* buf = json_dumps(json, 0);
    carrier_send(buf);
    free(buf);
    json_decref(json);
    LOG_INFO("Leave send_info()");
    return 0;
}
#else
static int send_info(char* token) {
    char buffer_model[MAX_MODEL_LENGTH];
    char buffer_version[MAX_VERSION_LENGTH];
    char buffer_wan_ip[64];
    char buffer_mac[13];
    json_t* json = json_pack("{s:s,s:s,s:s,s:s,s:s,s:i,s:i,s:s,s:i}",
            "type", "info",
            "model", tw_get_model(buffer_model),
            "mac", tw_get_mac(buffer_mac),
            "token", token,
            "ver", tw_get_version(buffer_version),
            "mem", tw_get_total_memory(),
            "storage", tw_get_total_storage(),
            "wan", tw_get_wan_ip(buffer_wan_ip),
            "uptime", tw_get_uptime());
    char* buf = json_dumps(json, 0);
    carrier_send(buf);
    free(buf);
    json_decref(json);
    LOG_INFO("Leave send_info()");
    return 0;
}

static void print_error_log(struct bufferevent* bev) {
    int err = bufferevent_socket_get_dns_error(bev);
    if (err) {
        LOG_ERROR("DNS error: %s", evutil_gai_strerror(err));
    }
    unsigned long ssl_error;
    while (0 != (ssl_error = bufferevent_get_openssl_error(bev))) {
        const char* msg = (const char*) ERR_reason_error_string(ssl_error);
        const char* lib = (const char*) ERR_lib_error_string(ssl_error);
        const char* func = (const char*) ERR_func_error_string(ssl_error);
        LOG_ERROR("SSL error: %s (%s) %s", lib, func, msg);
    }
}
#endif

/**
 * Process events from Carrier.
 *
 * <p>When connected, call all listeners on this event.
 * When error occurs or Carrier closes connection, try another Carrier.
 *
 * @param bev {@code bufferevent} of the connection where the event occurs
 * @param events the event code
 * @param ptr the token got from auth server
 */
static void on_event(struct bufferevent* bev, short events,
        void* ptr) {
    agent_ctx_t * c = get_agent_ctx();
    if (events & BEV_EVENT_CONNECTED) {
        evutil_make_socket_closeonexec(bufferevent_getfd(bev));
        c->conn_state = AGENT_CONNECTED;
        c->nr_failure = 0;
        connected = 1;
        evtimer_del(connect_timer);
        evtimer_add(keepalive_read_timer, &read_timeout);
        evtimer_add(keepalive_write_timer, &write_timeout);
        LOG_INFO("Connect okay.");
        dump_agent_ctx();
        send_info((char*) ptr);
    } else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        if (events & BEV_EVENT_ERROR) {
#ifndef COMPILE_TARGET_X86
            print_error_log(bev);
#else
            if (c->conn_state == AGENT_CONNECTING) {
                c->conn_state = AGENT_DISCONNECTED;
                c->disconn_reason = TCP_CONNECTION_ERROR;
                c->nr_failure++;
            } else if (c->conn_state != AGENT_UNKNOW) {
                c->conn_state = AGENT_UNKNOW;
                LOG_ERROR("A fetal error detected, agent state untracable");
            }
#endif
        } else {
            if (c->conn_state == AGENT_CONNECTED) {
                LOG_ERROR("Carrier closed connection.");
                c->conn_state = AGENT_DISCONNECTED;
                c->disconn_reason = TCP_CONNECTION_ERROR;
                c->nr_failure++;
            } else if (c->conn_state != AGENT_UNKNOW) {
                c->conn_state = AGENT_UNKNOW;
                LOG_ERROR("A fetal error detected, agent state untracable");
            }

        }
        evtimer_del(connect_timer);
        LOG_INFO("Re-connecting");
        reconnect();
    }
}

void carrier_free() {
    disconnect();
}

#ifndef COMPILE_TARGET_X86
void carrier_connect() {
    int err = auth_get_token_ex("cmagent", token);
    if (err) {
        LOG_ERROR("Failed to get token: %d %s", err, auth_get_last_error());
    }

    if ((bev = tcp_connect(token, names[c_index], port, on_data, on_event)) != NULL) {
        // A silent server who just ACK our "Client Hello" but not
        // send "Server Hello" in SSL negotiation phase will keep the
        // TCP connection in ESTABLISHED state, but we will never get
        // BEV_EVENT_CONNECTED event, because SSL connection is not
        // set up.
        // To be able to retry another connection, we start the timer
        // here, instead of the connected event callback.
        evtimer_add(connect_timer, &connect_timeout);
        LOG_DEBUG("No immediate error when connecting to Carrier %s", names[c_index]);
        return;
    }
    LOG_ERROR("Connect to Carrier %s failed.", names[c_index]);
}
#else
void carrier_connect() {
    agent_ctx_t *c = get_agent_ctx();
    if ((bev = tcp_connect(names[c_index], port, on_data, on_event)) != NULL) {
        // A silent server who just ACK our "Client Hello" but not
        // send "Server Hello" in SSL negotiation phase will keep the
        // TCP connection in ESTABLISHED state, but we will never get
        // BEV_EVENT_CONNECTED event, because SSL connection is not
        // set up.
        // To be able to retry another connection, we start the timer
        // here, instead of the connected event callback.
        c->conn_state = AGENT_CONNECTING;
        evtimer_add(connect_timer, &connect_timeout);
        LOG_DEBUG("No immediate error when connecting to Carrier %s", names[c_index]);
    } else {
        c->conn_state = AGENT_DISCONNECTED;
        c->disconn_reason = TCP_CONNECTION_ERROR;
        c->retry_tm = time(NULL);
        ++c->nr_failure;
        LOG_ERROR("Connect to Carrier %s failed.", names[c_index]);
    }
}
#endif

void carrier_send(char* message) {
    evtimer_add(keepalive_write_timer, &write_timeout);
    int len = strlen(message);
    // Libevent uses a single thread, so we don't need lock here.
    struct evbuffer* output = bufferevent_get_output(bev);
    // Convert to network endian (big-endian).
    uint32_t net_size = htonl(len);
    evbuffer_add(output, &net_size, PREFIX_LENGTH);
    evbuffer_add(output, message, len);
    LOG_DEBUG("Leave carrier_send(%s)", message);
}

static void carrier_reload(evutil_socket_t fd __attribute__((unused)),
        short events __attribute__((unused)), void* ptr __attribute__((unused))) {
    agent_ctx_t * c = get_agent_ctx();
    if (c->conn_state == AGENT_CONNECTED) {
        LOG_INFO("Start reloading...");
        disconnect();
        c->conn_state = AGENT_DISCONNECTED;
        c->disconn_reason = RELOADING;
        c->nr_failure = 0;
        carrier_connect();
        LOG_INFO("Reloaded.");
    } else {
        LOG_INFO("Ignore reloading when disconnected.");
    }
}

#ifndef COMPILE_TARGET_X86
void carrier_init(struct event_base* base) {
    reload_event = evsignal_new(base, SIGHUP, carrier_reload, NULL);
    evsignal_add(reload_event, NULL);
    idle_timer = evtimer_new(base, call, carrier_connect);
    connect_timer = evtimer_new(base, call, reconnect);
    keepalive_read_timer = evtimer_new(base, call, reconnect);
    keepalive_write_timer = evtimer_new(base, call, send_status);
}
#else
void carrier_init(struct event_base* base) {
    reload_event = evsignal_new(base, SIGHUP, carrier_reload, NULL);
    evsignal_add(reload_event, NULL);
    idle_timer = evtimer_new(base, call, carrier_connect);
    connect_timer = evtimer_new(base, call, connect_timeout_cb);
    keepalive_read_timer = evtimer_new(base, call, keepalive_read_timeout_cb);
    keepalive_write_timer = evtimer_new(base, call, send_status);
}
#endif
