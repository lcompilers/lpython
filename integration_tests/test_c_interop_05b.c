#include "test_c_interop_05b.h"

int32_t f_i32_i32(int32_t x) {
    return x+1;
}

int32_t f_pi32_i32(int32_t *x) {
    return *x+1;
}

int32_t driver1() {
    return callback1();
}

int32_t driver2() {
    int32_t x;
    x = 3;
    return callback2(x);
}

int32_t driver3() {
    int32_t x;
    x = 3;
    return callback3(&x);
}
