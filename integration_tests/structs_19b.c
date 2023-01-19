#include <stdlib.h>
#include <complex.h>
#include "structs_19b.h"


struct buffer_c {
    int8_t buffer8;
    int32_t buffer1;
    float buffer2;
    float complex buffer3;
    double buffer4;
    int16_t buffer5;
    int64_t buffer6;
    double complex buffer7;
};

struct buffer_c_array {
    int32_t buffer1[32];
};

void fill_buffer(void* buffer_cptr) {
    struct buffer_c* buffer_clink_ = (struct buffer_c*) buffer_cptr;
    buffer_clink_->buffer8 = 8;
    buffer_clink_->buffer1 = 9;
    buffer_clink_->buffer2 = 10.0;
    buffer_clink_->buffer3 = CMPLXF(14.0, 15.0);
    buffer_clink_->buffer4 = 11.0;
    buffer_clink_->buffer5 = 12;
    buffer_clink_->buffer6 = 13;
    buffer_clink_->buffer7 = CMPLXL(16.0, 17.0);
}

void fill_buffer_array(void* buffer_cptr) {
    struct buffer_c_array* buffer_clink_ = (struct buffer_c_array*) buffer_cptr;
    for( int i = 0; i < 32; i++ ) {
        buffer_clink_->buffer1[i] = i + 1;
    }
}

void* get_buffer() {
    return malloc(sizeof(struct buffer_c));
}

void* get_buffer_array() {
    return malloc(sizeof(struct buffer_c_array));
}
