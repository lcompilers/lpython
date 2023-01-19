#include <stdlib.h>
#include <complex.h>
#include "structs_20b.h"


struct __attribute__((packed)) buffer_c_packed {
    int8_t buffer8[32];
    int16_t buffer1[32];
    int32_t buffer2;
    float buffer3[32];
    double buffer4;
    float complex buffer5[32];
    double complex buffer6;
};

void fill_buffer(void* buffer_cptr) {
    struct buffer_c_packed* buffer_clink_ = (struct buffer_c_packed*) buffer_cptr;
    for( int i = 0; i < 32; i++ ) {
        buffer_clink_->buffer8[i] = i + 8;
    }
    for( int i = 0; i < 32; i++ ) {
        buffer_clink_->buffer1[i] = i + 1;
    }
    buffer_clink_->buffer2 = 3;
    for( int i = 0; i < 32; i++ ) {
        buffer_clink_->buffer3[i] = i + 3;
    }
    buffer_clink_->buffer4 = 5;
    for( int i = 0; i < 32; i++ ) {
        buffer_clink_->buffer5[i] = CMPLXF(i + 5, 0.0);
    }
    buffer_clink_->buffer6 = CMPLXL(7, 0.0);
}

void* get_buffer() {
    return malloc(sizeof(struct buffer_c_packed));
}
