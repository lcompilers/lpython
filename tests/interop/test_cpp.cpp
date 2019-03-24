#include <cstddef>
#include <iostream>


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
