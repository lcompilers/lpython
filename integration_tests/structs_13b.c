#include <stdlib.h>

void* cmalloc(int64_t size) {
    return malloc(size);
}

int32_t is_null(void* ptr) {
    return ptr == NULL;
}
