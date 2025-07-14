#ifndef TEST_C_INTEROP_02B
#define TEST_C_INTEROP_02B

#include <stdint.h>

typedef struct {
    char* x;
    int64_t size;
    int64_t capacity;
} string_descriptor;

double  f_f64_f64(double  x);
float   f_f32_f32(float   x);
int64_t f_i64_i64(int64_t x);
int32_t f_i32_i32(int32_t x);
int16_t f_i16_i16(int16_t x);
int8_t  f_i8_i8  (int8_t  x);
int32_t f_str_i32(string_descriptor x);


#endif // TEST_C_INTEROP_02B
