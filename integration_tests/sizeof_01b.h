#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

void* cmalloc(int64_t bytes);
bool cfree(void* x);
void fill_carray(void* arr, int32_t size);
int64_t sum_carray(void* arr, int32_t size);
