#ifndef _LOG_H_
#define _LOG_H_

static enum loglevel {
    WARN = 0,
    INFO,
    ERROR,
    DEBUG,
};

extern void __log(const char *file, const char *fn, int line, int level, const char *fmt, ...);
#define warn_log(fmt...) do { __log(__FILE__, __FUNCTION__, __LINE__, WARN, ##fmt); } while(0)
#define error_log(fmt...) do { __log(__FILE__, __FUNCTION__, __LINE__, ERROR, ##fmt); } while(0)
#define debug_log(fmt...) do { __log(__FILE__, __FUNCTION__, __LINE__, DEBUG, ##fmt); } while(0)
#define info_log(fmt...) do { __log(__FILE__, __FUNCTION__, __LINE__, INFO, ##fmt); } while(0)
#endif
