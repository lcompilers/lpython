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
descriptor<Rank, Type> c_desc(Type *array_ptr,
        const std::vector<std::pair<int,int>> &dims) {
    descriptor<Rank, Type> a;
    a.base_addr = array_ptr;
    a.offset = 0;
    for (int n=0; n < Rank; n++) {
        a.dim[n].lower_bound = dims[n].first;
        a.dim[n].upper_bound = dims[n].second;

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

#include "mod2.h"

TEST_CASE("f1")
{
    int a = 1;
    int b = 2;
    CHECK(__mod1_MOD_f1(&a, &b) == 3);
}

TEST_CASE("f2")
{
    int n = 3;
    std::vector<int32_t> data(3);
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    CHECK(__mod1_MOD_f2(&n, &data[0]) == 6);
}

TEST_CASE("f2b")
{
    std::vector<int32_t> data(3);
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;

    descriptor<1, int32_t> a = c_desc<1, int32_t>(&data[0],
            {{1, 3}});
    CHECK(__mod1_MOD_f2b(&a) == 6);
}

TEST_CASE("f3")
{
    std::vector<int32_t> data(6);
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    data[5] = 6;

    int n = 2;
    int m = 3;

    CHECK(__mod1_MOD_f3(&n, &m, &data[0]) == 21);
}

TEST_CASE("f3b")
{
    std::vector<int32_t> data(6);
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    data[5] = 6;

    descriptor<2, int32_t> a = c_desc<2, int32_t>(&data[0],
            {{1, 2}, {1, 3}});

    CHECK(__mod1_MOD_f3b(&a) == 21);
}

TEST_CASE("f4")
{
    int a = 1;
    int b = 0;
    CHECK(__mod1_MOD_f4(&a, &b) == 0);
    CHECK(b == 1);
}

TEST_CASE("f5b")
{
    std::vector<int32_t> data(6);
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    data[5] = 6;

    descriptor<2, int32_t> a = c_desc<2, int32_t>(&data[0],
            {{1, 2}, {1, 3}});

    CHECK(__mod1_MOD_f5b(&a) == 9);
}
