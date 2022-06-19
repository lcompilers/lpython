#include "test_c_interop_05b.h"

int32_t f_i32_i32(int32_t x) {
    return x+1;
}

int32_t f_pi32_i32(int32_t *x) {
    return *x+1;
}

int32_t f_pstruct_i32(struct Data *x) {
    return x->a+1;
}

int32_t f_enum_i32(enum MyEnum x) {
    return (int32_t)x + 2;
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

int32_t driver4() {
    struct Data x;
    x.a = 3;
    return callback4(&x);
}

int32_t driver5() {
    enum MyEnum x;
    x = M_c;
    return callback5(x);
}
