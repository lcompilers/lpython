#include "sizeof_01b.h"
#include <stdio.h>

void* cmalloc(int64_t bytes) {
    return malloc(bytes);
}

bool cfree(void* x) {
    free(x);
    return 1;
}

void fill_carray(void* arr, int32_t size) {
    int64_t* arr_int = arr;
    for( int32_t i = 0; i < size; i++ ) {
        arr_int[i] = i + 1;
    }
}

int64_t sum_carray(void* arr, int32_t size) {
    int64_t* arr_int = arr;
    int64_t sum = 0;
    for( int32_t i = 0; i < size; i++ ) {
        sum += arr_int[i];
    }
    return sum;
}
