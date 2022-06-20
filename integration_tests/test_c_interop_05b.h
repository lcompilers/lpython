#ifndef TEST_C_INTEROP_05B
#define TEST_C_INTEROP_05B

#include <stdint.h>

struct Data {
    int64_t x, *p;
    double y, *z;
    int32_t a;
    char name[25];
};

enum MyEnum {
    M_a = 0,
    M_b = 1,
    M_c = 2,
    M_d = 3,
};

int32_t f_i32_i32(int32_t x);
int32_t f_pi32_i32(int32_t *x);
int32_t f_pstruct_i32(struct Data *x);
int32_t f_enum_i32(enum MyEnum x);

int32_t callback1();
int32_t callback2(int32_t x);
int32_t callback3(int32_t *x);
int32_t callback4(struct Data *x);
int32_t callback5(enum MyEnum x);

int32_t driver1();
int32_t driver2();
int32_t driver3();
int32_t driver4();
int32_t driver5();


#endif // TEST_C_INTEROP_05B
