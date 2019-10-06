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

template <int Rank, typename Type>
descriptor<Rank, Type> c_desc(Type *array_ptr) {
    descriptor<Rank, Type> a;
    a.base_addr = array_ptr;
    a.dim[0].lower_bound = 1;
    a.dim[0].upper_bound = 2;
    a.dim[1].lower_bound = 1;
    a.dim[1].upper_bound = 3;
    a.offset = 0;
    for (int n=0; n < Rank; n++) {
        if (n == 0) {
            a.dim[n].stride = 1;
        } else {
            a.dim[n].stride =
                (a.dim[n-1].upper_bound - a.dim[n-1].lower_bound + 1)
                * a.dim[n-1].stride;
        }
        a.offset = a.offset - a.dim[n].lower_bound * a.dim[n].stride;
    }
    a.dtype = Rank;
    // 1 is INTEGER element type
    a.dtype |= (1 << 3);
    // sizeof(Type) is the size of INTEGER
    a.dtype |= (sizeof(Type) << 6);
    return a;
}

extern "C" {

int32_t __mod1_MOD_f2b(descriptor<1, int32_t> *descr);
int32_t __mod1_MOD_f3b(descriptor<2, int32_t> *descr);

}

TEST_CASE("f2b")
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

    CHECK(__mod1_MOD_f2b(&a) == 6);
}

TEST_CASE("f3b")
{
    int32_t *data = new int32_t[3];
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    data[5] = 6;

    descriptor<2, int32_t> a = c_desc<2, int32_t>(data);

    CHECK(__mod1_MOD_f3b(&a) == 21);
}
