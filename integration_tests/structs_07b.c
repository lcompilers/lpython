#include "structs_07b.h"

void* cmalloc(int64_t bytes) {
    return malloc(bytes);
}

bool cfree(void* x) {
    free(x);
    return 1;
}
