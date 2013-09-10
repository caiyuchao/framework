#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include "lock.h"


static FILE * log = NULL;
static pthread_mutex_t lock;


static char *log_level[] = {
    "W",                    //Warning
    "I",                    //Info
    "E",                    //Error
    "D"                     //Debug
};

void __log(const char *file, const char *fn, int line, int level, const char *fmt, ...)
{
    char timestr[64];
    char str[512];
    struct tm *tm = NULL;
    time_t tim;
    va_list ap;
    int ret;

    time(&tim);
    tm = localtime(&tim);
    strftime(timestr, 64, "%F %T", tm);
    va_start(ap, fmt);

    ret = vsnprintf(str, 512, fmt, ap);
    assert (ret > 0 &&  ret < 512);

    pthread_mutex_lock(&lock);
    if (log) {
        fprintf(log, "[%s] %s [%s:%s:%d] %s", timestr, log_level[level], file, fn, line, str);
    } else {
        fprintf(stderr, "[%s] %s [%s:%s:%d] %s", timestr, log_level[level], file, fn, line, str);
    }
    fflush(log);
    pthread_mutex_unlock(&lock);
}

int log_init(const char *logfile)
{
    int fd;
    if (access(logfile, F_OK) == -1) {
        fd = open(logfile, O_CREAT | O_WRONLY);
        if (fd < 0) {
            return -1;
        }
        close(fd);
        log = fopen(logfile, "a+");
    } else {
        log = fopen(logfile, "a+");
    }
    if (log == NULL) {
        return -1;
    }
    pthread_mutex_init(&lock, NULL);
    return 0;
}

