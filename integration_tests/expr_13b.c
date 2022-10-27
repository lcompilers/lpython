#include "expr_13b.h"
#include <stdlib.h>
#include <stdio.h>

void* deref_array(void** x, int32_t idx) {
    return x[idx];
}

void** get_arrays(int32_t num_arrays) {
    int32_t** mat = malloc(sizeof(int32_t*) * num_arrays);
    for (int32_t i = 0; i < num_arrays; i++) {
        mat[i] = malloc(sizeof(int32_t) * 10);
    }
    return (void**) mat;
}

int32_t sum_array(void* x, int32_t size) {
    int32_t* x_int = x;
    int32_t sum = 0;
    for (int32_t i = 0; i < size; i++) {
        sum += x_int[i];
    }
    return sum;
}
