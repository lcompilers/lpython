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
    int8_t buffer8[100];
    int32_t buffer1[32];
    float buffer2[32];
    double buffer4[32];
    int16_t buffer5[64];
    int64_t buffer6[16];
    float complex buffer3[16];
    double complex buffer7[8];
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
    for( int i = 0; i < 128; i++ ) {
        buffer_clink_->buffer8[i] = i + 8;
    }
    for( int i = 0; i < 32; i++ ) {
        buffer_clink_->buffer1[i] = i + 1;
        buffer_clink_->buffer2[i] = i + 2;
        buffer_clink_->buffer4[i] = i + 4;
    }
    for( int i = 0; i < 64; i++ ) {
        buffer_clink_->buffer5[i] = i + 5;
    }
    for( int i = 0; i < 16; i++ ) {
        buffer_clink_->buffer6[i] = i + 6;
        buffer_clink_->buffer3[i] = CMPLXF(i + 3, 0.0);
    }
    for( int i = 0; i < 8; i++ ) {
        buffer_clink_->buffer7[i] = CMPLXL(i + 7, 0.0);
    }
}

void* get_buffer() {
    return malloc(sizeof(struct buffer_c));
}

void* get_buffer_array() {
    return malloc(sizeof(struct buffer_c_array));
}
