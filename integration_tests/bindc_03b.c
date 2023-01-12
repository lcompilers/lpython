#include "bindc_03b.h"
#include <stdlib.h>

void g(int32_t* x, int32_t value, bool offset_value) {
    if( offset_value ) {
        *x = value + 1;
    } else {
        *x = value;
    }
}

void* get_array(int32_t size) {
    return malloc(size * sizeof(int32_t));
}
