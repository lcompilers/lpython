#include <cstddef>
#include <iostream>

/*
  GFortran ABI is stable for a given GFortran module file version. The module
  file version stays the same between releases (4.5.0 and 4.5.2), but usually
  changes between minor versions (4.5 vs. 4.6). Here is a correspondence of
  GFortran compiler versions and GFortran module file versions:

  GFortran version    module file version
  ---------------------------------------
  <= 4.3              unversioned
  4.4 (Apr 2009)       0
  4.5 (Apr 2010)       4
  4.6 (Mar 2011)       6
  4.7 (Mar 2012)       9
  4.8 (Mar 2013)      10
  4.9 (Apr 2014)      12
  5.x (Apr 2015)      14
  6.x (Apr 2016)      14
  7.x (May 2017)      14
  8.x (May 2018)      15
  9.x (??? 2019)      ??

  The GFortran array descriptor is defined in libgfortran.h.
  Module file version 15:

  https://github.com/gcc-mirror/gcc/blob/gcc-8_1_0-release/libgfortran/libgfortran.h#L324

    typedef struct descriptor_dimension
    {
        index_type _stride;
        index_type lower_bound;
        index_type _ubound;
    }
    descriptor_dimension;

    typedef struct dtype_type
    {
        size_t elem_len;
        int version;
        signed char rank;
        signed char type;
        signed short attribute;
    }
    dtype_type;

    #define GFC_FULL_ARRAY_DESCRIPTOR(r, type) \
    struct {\
        type *base_addr;\
        size_t offset;\
        dtype_type dtype;\
        index_type span;\
        descriptor_dimension dim[r];\
    }

  Module file versions 0..14 (changes from 15: no `span` member, `dtype` is
  an integer instead of a struct):

  https://github.com/gcc-mirror/gcc/blob/gcc-7_1_0-release/libgfortran/libgfortran.h#L328
  https://github.com/gcc-mirror/gcc/blob/gcc-4_4_0-release/libgfortran/libgfortran.h#L298

    typedef struct descriptor_dimension
    {
        index_type _stride;
        index_type lower_bound;
        index_type _ubound;
    }
    descriptor_dimension;

    #define GFC_ARRAY_DESCRIPTOR(r, type) \
    struct {\
        type *base_addr;\
        size_t offset;\
        index_type dtype;\
        descriptor_dimension dim[r];\
    }

  Then there is the CFI array descriptor from ISO_Fortran_binding.h:

  https://github.com/gcc-mirror/gcc/blob/c3ce5d657bac95a3ac5c0457ac48b47a9710c838/libgfortran/ISO_Fortran_binding.h#L69

    typedef struct CFI_dim_t
    {
        CFI_index_t lower_bound;
        CFI_index_t extent;
        CFI_index_t sm;
    }
    CFI_dim_t;

    #define CFI_CDESC_TYPE_T(r, base_type) \
        struct { \
            base_type *base_addr; \
            size_t elem_len; \
            int version; \
            CFI_rank_t rank; \
            CFI_attribute_t attribute; \
            CFI_type_t type; \
            CFI_dim_t dim[r]; \
    }

  GFortran then converts between its own array descriptor and the CFI
  descriptor, for example here is the code that converts from CFI to
  GFortran:

  https://github.com/gcc-mirror/gcc/blob/c3ce5d657bac95a3ac5c0457ac48b47a9710c838/libgfortran/runtime/ISO_Fortran_binding.c#L37

*/

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

int __mod1_MOD_f2b(descriptor<1, int> *descr);

}

int main()
{
    descriptor<1, int> a;
    a.base_addr = new int[3];
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
    std::cout << "Result: " << s << std::endl;
    return 0;
}
