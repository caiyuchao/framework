/*
 * Copyright (c) 2012 Elite Co., Ltd.
 * Author: Hong Shen <sh@ikwcn.com>
 */

#include <event2/buffer.h>
#include <stdlib.h>

#include "frame_decoder.h"
#include "log.h"

static const unsigned int PREFIX_LENGTH = 4;
static const unsigned int MAX_MESSAGE_LENGTH = 32768;

void frame_decoder_read(struct evbuffer* input, frame_decoder_callback cb,
        error_callback error_cb) {
    while (1) {
        // Convert to host endian.
        uint32_t net_size;
        int tmp = evbuffer_copyout(input, &net_size, PREFIX_LENGTH);
        // We never called evbuffer_freeze(), so "tmp" always >= 0.
        // See the source code of evbuffer_freeze() and evbuffer_copyout().
        if ((unsigned int)tmp < PREFIX_LENGTH) {
            break; // Message incomplete. Waiting for more bytes.
        }
        uint32_t length = ntohl(net_size);
        if (length > MAX_MESSAGE_LENGTH) {
            // Avoid illegal data (too large message) crashing this client.
            LOG_ERROR("Message too large: %u", length);
            error_cb();
            break;
        }
        if (evbuffer_get_length(input) < PREFIX_LENGTH + length) {
            break; // Message incomplete. Waiting for more bytes.
        }
        evbuffer_drain(input, PREFIX_LENGTH);
        char* buf = malloc(length + 1); // Add space for trailing '\0'.
        evbuffer_remove(input, buf, length);
        buf[length] = '\0';
        cb(buf);
        free(buf);
    }
}
