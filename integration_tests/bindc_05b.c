#include "bindc_05b.h"
#include <stdio.h>

void* trunc_custom(void** value) {
    float* value_float = *value;
    int trunced = *value_float;
    *value_float = trunced;
    return *value;
}

void print_value(float* value) {
    printf("print_value: %f\n", *value);
}
