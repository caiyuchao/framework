/*
 * Copyright (c) 2013 Elite Co., Ltd.
 * Author: Hong Shen <sh@ikwcn.com>
 */

#include <event2/event.h>
#include <event2/util.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "cron.h"

static struct event* timer;

static void shell_execute() {
    char lua_path[] = "/usr/bin/lua";
    char full_path[] = "/usr/lib/cmagent/cron.lua";
    pid_t pid = fork();
    if (pid < 0) {
        return;
    }
    if (pid == 0) {
        execl(lua_path, lua_path, full_path, (char *) 0);
        exit(1);
    }
}

static void cron_callback() {
    // Use double forks to deal with zombie process.
    pid_t pid = fork();
    if (pid < 0) {
        return;
    }
    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else if (pid == 0) {
        shell_execute();
        exit(0);
    }
}

static void repeat_call(evutil_socket_t unused1 __attribute__((unused)),
        short unused2 __attribute__((unused)), void* arg) {
    cron_callback();
    if (timer == NULL) {
        timer = evtimer_new(arg, repeat_call, arg);
    }
    struct timeval one_minute = {60, 0};
    evtimer_add(timer, &one_minute);
}

void cron_init(struct event_base* base) {
#ifndef COMPILE_TARGET_X86
    repeat_call(0, 0, base);
#endif
}
