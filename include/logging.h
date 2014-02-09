#ifndef _LOGGING_H_
#define _LOGGING_H_

enum loglevel {
    WARN = 0,
    INFO,
    ERROR,
    DEBUG,
    NONE,
};

extern void __log(const char *file, const char *fn, int line, int level, const char *fmt, ...);
extern int init_log(const char *);
#define LOG_WARN(fmt...) do { __log(__FILE__, __FUNCTION__, __LINE__, WARN, ##fmt); } while(0)
#define LOG_ERROR(fmt...) do { __log(__FILE__, __FUNCTION__, __LINE__, ERROR, ##fmt); } while(0)
#define LOG_DEBUG(fmt...) do { __log(__FILE__, __FUNCTION__, __LINE__, DEBUG, ##fmt); } while(0)
#define LOG_INFO(fmt...) do { __log(__FILE__, __FUNCTION__, __LINE__, INFO, ##fmt); } while(0)

#endif
