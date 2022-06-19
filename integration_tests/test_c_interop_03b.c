#include "test_c_interop_03b.h"

int32_t f_pi32_i32(int32_t *x) {
    return *x+1;
}

int64_t f_pi64_i32(int64_t *x) {
    return *x+1;
}

float   f_pf32_i32(float *x) {
    return *x+1;
}

double  f_pf64_i32(double *x) {
    return *x+1;
}

void*  f_pvoid_pvoid(void *x) {
    return x;
}
