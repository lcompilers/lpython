#include <string.h>

#include "test_c_interop_02b.h"

double f_f64_f64(double x) {
    return x+1;
}

float f_f32_f32(float x) {
    return x+1;
}

int64_t f_i64_i64(int64_t x) {
    return x+1;
}

int32_t f_i32_i32(int32_t x) {
    return x+1;
}

int16_t f_i16_i16(int16_t x) {
    return x+1;
}

int8_t f_i8_i8(int8_t x) {
    return x+1;
}

int32_t f_str_i32(string_descriptor x) {
    return x.size;
}
