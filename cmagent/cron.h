/*
 * Copyright (c) 2013 Elite Co., Ltd.
 * Author: Hong Shen <sh@ikwcn.com>
 */

#ifndef CRON_H_
#define CRON_H_

struct event_base;

void cron_init(struct event_base* base);

#endif /* CRON_H_ */
