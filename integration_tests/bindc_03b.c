#include "bindc_03b.h"
#include <stdlib.h>

void g(int32_t* x, int32_t value) {
    *x = value;
}

void* get_array(int32_t size) {
    return malloc(size * sizeof(int32_t));
}
