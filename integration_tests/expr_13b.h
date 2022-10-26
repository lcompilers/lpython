
#ifndef EXPR_13B
#define EXPR_13B

#include <inttypes.h>

void* deref_array(void** x, int32_t idx);
void** get_arrays(int32_t num_arrays);
int32_t sum_array(void* x, int32_t size);

#endif
