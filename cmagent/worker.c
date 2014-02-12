/*
 * Copyright (c) 2012 Elite Co., Ltd.
 * Author: Hong Shen <sh@ikwcn.com>
 */

#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <cmagent.h>
#include <stdlib.h>
#include <logging.h>
#include <fcntl.h>
#include "carrier.h"
#include "cron.h"
#include "net.h"

#define CTX_FILE "/tmp/cmagent"
agent_ctx_t agent_ctx;

static char *agent_state_string[] = {
    "connected",
    "connecting",
    "disconnected",
    "unknow",
    NULL
};

static char *disconn_reason_string[] = {
    "ssl connection error",
    "tcp connection error",
    "connection initializing",
    "cmagent reloading",
    "keepalive error",
    NULL
};


void dump_agent_ctx(void)
{
    agent_ctx_t  *c = get_agent_ctx();
    struct tm tmp_tm;
    if (c->ctx_file == NULL) {
        return;
    }
    ftruncate(fileno(c->ctx_file), 0);
    fseek(c->ctx_file, SEEK_SET, 0);
    fprintf(c->ctx_file, "state:%s\n", get_agent_state_string(c->conn_state));
    if (c->conn_state != AGENT_DISCONNECTED) {
        fflush(c->ctx_file);
        return ;
    }
    fprintf(c->ctx_file, "reason:%s\n", get_disconn_reason_string(c->disconn_reason));
    localtime_r(&c->retry_tm, &tmp_tm);
    fprintf(c->ctx_file, "reconnect_at:%02d:%02d:%02d\n", tmp_tm.tm_hour, tmp_tm.tm_min, tmp_tm.tm_sec);
    fprintf(c->ctx_file, "failures:%d\n", c->nr_failure);
    fflush(c->ctx_file);
}

static void agent_ctx_init(agent_ctx_t *c)
{
    c->retry_tm = time(NULL);
    c->conn_state = AGENT_DISCONNECTED;
    c->disconn_reason = CONNECTION_INITIALIZING;
    c->nr_failure = 0;
    c->ctx_file = fopen(CTX_FILE, "w+");
    if (c->ctx_file == NULL) {
        LOG_ERROR("Failed to open file %s", CTX_FILE);
    }
}

const char *get_agent_state_string(int state)
{
    if (state < AGENT_CONNECTED || state > AGENT_UNKNOW) {
        return NULL;
    } 
    return agent_state_string[state];
}


const char *get_disconn_reason_string(int reason)
{
    if (reason < SSL_CONNECTION_ERROR || reason > KEEPALIVE_ERROR) {
        return NULL;
    } 
    return disconn_reason_string[reason];
}

/**
 * The entry point of Worker daemon, which keeps running.
 *
 * @return 0 on success, -1 on failure
 */
int cmagent(int argc, char **argv) {
//    time_t tm;
//    struct tm tmp_tm;
//    tm = time(NULL);
//    localtime_r(&tm, &tmp_tm);
//    LOG_INFO("%d-%d-%d\n", tmp_tm.tm_hour, tmp_tm.tm_min, tmp_tm.tm_sec);
//    return 0;
    signal(SIGPIPE, SIG_IGN);
    struct event_base* base = net_init();
    if (base == NULL) {
        return -1;
    }
    cron_init(base);
    agent_ctx_init(get_agent_ctx());
    carrier_init(base);
    carrier_connect();
    net_dispatch();

    // Clean up.
    carrier_free();
    net_free();
    return 0;
}
