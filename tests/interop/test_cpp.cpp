// ptrdiff_t, size_t
#include <cstddef>

// int32_t
#include <cstdint>

#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <tests/doctest.h>


struct descriptor_dimension
{
  ptrdiff_t stride;
  ptrdiff_t lower_bound;
  ptrdiff_t upper_bound;
};

template <int Rank, typename Type>
struct descriptor {
  Type *base_addr;
  size_t offset;
  ptrdiff_t dtype;
  descriptor_dimension dim[Rank];
};

extern "C" {

int32_t __mod1_MOD_f2b(descriptor<1, int32_t> *descr);

}

TEST_CASE("main")
{
    descriptor<1, int32_t> a;
    a.base_addr = new int32_t[3];
    a.base_addr[0] = 1;
    a.base_addr[1] = 2;
    a.base_addr[2] = 3;
    a.dim[0].stride = 1;
    a.dim[0].lower_bound = 1;
    a.dim[0].upper_bound = 3;
    a.dtype = 1;
    // 1 is INTEGER element type
    a.dtype |= (1 << 3);
    // 4 is the size of INTEGER
    a.dtype |= (4 << 6);
    a.offset = - (a.dim[0].lower_bound * a.dim[0].stride);

    int s;
    s = __mod1_MOD_f2b(&a);
    CHECK(s == 6);
}
