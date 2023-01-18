#include <stdlib.h>

#include "structs_18b.h"


struct __attribute__((packed)) __attribute__((ms_struct)) buffer_c {
    int64_t x;
};

void fill_buffer(void* buffer_cptr) {
    struct buffer_c* buffer_clink_ = (struct buffer_c*) buffer_cptr;
    buffer_clink_->x = 8;
}

struct buffer_c *x;

void* get_buffer() {
    return malloc(sizeof(struct buffer_c));
}
