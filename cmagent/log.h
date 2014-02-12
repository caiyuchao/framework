/*
 * Copyright (c) 2012 Elite Co., Ltd.
 * Author: Hong Shen <sh@ikwcn.com>
 */

#ifndef LOG_H_
#define LOG_H_

#ifndef COMPILE_TARGET_X86
#include <hwf_log.h>

#define LOG_DEBUG(expr ...) hwf_log(DBG, __func__, __LINE__, expr)
#define LOG_INFO(expr ...) hwf_log(LOG, __func__, __LINE__, expr)
#define LOG_ERROR(expr ...) hwf_log(ERR, __func__, __LINE__, expr)
#else
#include <logging.h>
#endif

#endif /* LOG_H_ */
