#pragma once

#include <stdint.h>

// https://embedjournal.com/implementing-circular-buffer-embedded-c/

typedef struct {
    uint8_t *const buffer;
    int head;
    int tail;
    const int maxlen;
} circ_bbuf_t;

int32_t circ_bbuf_push(circ_bbuf_t *c, uint8_t data);
int32_t circ_bbuf_pop(circ_bbuf_t *c, uint8_t *data);
