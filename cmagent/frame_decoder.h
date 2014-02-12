/*
 * Copyright (c) 2012 Elite Co., Ltd.
 * Author: Hong Shen <sh@ikwcn.com>
 */

#ifndef FRAME_DECODER_H_
#define FRAME_DECODER_H_

struct evbuffer;

typedef int (*frame_decoder_callback)(char* buf);
typedef void (*error_callback)();

void frame_decoder_read(struct evbuffer* input, frame_decoder_callback cb,
    error_callback error_cb);

#endif /* FRAME_DECODER_H_ */
