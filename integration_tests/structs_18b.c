#include "structs_18b.h"
#include <complex.h>

int16_t sum_buffer_i16(int16_t data[], int32_t size) {
    int16_t sum_buffer = 0;
    for( int32_t i = 0; i < size; i++ ) {
        sum_buffer += data[i];
    }
    return sum_buffer;
}

struct __attribute__((packed)) buffer_c {
    int8_t buffer8[32];
    int32_t buffer1[32];
    float buffer2[32];
    float complex buffer3[32];
    double buffer4[32];
    int16_t buffer5[32];
    int64_t buffer6[32];
    double complex buffer7[32];
};

void fill_buffer(void* buffer_cptr) {
    struct buffer_c* buffer_clink_ = (struct buffer_c*) buffer_cptr;
    for(int32_t i = 0; i < 32; i++) {
        buffer_clink_->buffer8[i] = i + 2;
        buffer_clink_->buffer1[i] = i + 4;
        buffer_clink_->buffer2[i] = i + 6;
        buffer_clink_->buffer3[i] = CMPLXF(i + 8, 0);
        buffer_clink_->buffer4[i] = i + 10;
        buffer_clink_->buffer5[i] = i + 12;
        buffer_clink_->buffer6[i] = i + 14;
        buffer_clink_->buffer7[i] = CMPLXL(i + 16, 0);
    }
}
