#include <stdlib.h>

void* cmalloc(int64_t size) {
    return malloc(size);
}

int32_t is_null(void* ptr) {
    return ptr == NULL;
}

int32_t add_Aptr_members(int32_t Ax, int16_t Ay) {
    return Ax + Ay;
}
