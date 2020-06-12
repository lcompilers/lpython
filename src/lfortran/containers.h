#ifndef LFORTRAN_CONTAINERS_H
#define LFORTRAN_CONTAINERS_H

#include <cstring>
#include <lfortran/parser/alloc.h>

namespace LFortran
{

// Vector implementation
template <typename T>
struct Vec {
    size_t n, max;
    T* p;

    // reserve() must be called before calling push_back()
    void reserve(Allocator &al, size_t max) {
        n = 0;
        this->max = max;
        p = al.allocate<T>(max);
    }

    void push_back(Allocator &al, T x) {
        if (n == max) {
            size_t max2 = 2*max;
            T* p2 = al.allocate<T>(max2);
            std::memcpy(p2, p, sizeof(T) * max);
            p = p2;
            max = max2;
        }
        p[n] = x;
        n++;
    }

    size_t size() const {
        return n;
    }

    size_t capacity() const {
        return max;
    }

    // Returns a copy of the data as std::vector
    std::vector<T> as_vector() const {
        return std::vector<T>(p, p+n);
    }
};

static_assert(std::is_standard_layout<Vec<int>>::value);
static_assert(std::is_trivial<Vec<int>>::value);

// String implementation (not null-terminated)
struct Str {
    size_t n;
    char* p;

    // Returns a copy of the string as a NULL terminated std::string
    std::string str() const { return std::string(p, n); }

    // Returns a copy of the string as a NULL terminated C string,
    // allocated using Allocator
    char* c_str(Allocator &al) const {
        char *s = al.allocate<char>(n+1);
        std::memcpy(s, p, sizeof(char) * n);
        s[n] = '\0';
        return s;
    }

    size_t size() const {
        return n;
    }
};

static_assert(std::is_standard_layout<Str>::value);
static_assert(std::is_trivial<Str>::value);

} // namespace LFortran




#endif
