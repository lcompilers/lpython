#ifndef LFORTRAN_PARSER_ALLOC_H
#define LFORTRAN_PARSER_ALLOC_H

#include <algorithm>
#include <stdexcept>
#include <new>
#include <cstdlib>

#include <lfortran/assert.h>

#define ALIGNMENT 8

inline size_t align(size_t n) {
  return (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

class Allocator
{
    void *start;
    size_t current_pos;
    size_t size;
public:
    Allocator(size_t s) {
        s += ALIGNMENT;
        start = malloc(s);
        if (start == nullptr) throw std::runtime_error("malloc failed.");
        current_pos = (size_t)start;
        current_pos = align(current_pos);
        size = s;
    }
    ~Allocator() {
        if (start != nullptr) free(start);
    }

    // Allocates `s` bytes of memory, returns a pointer to it
    void *alloc(size_t s) {
        LFORTRAN_ASSERT(start != nullptr);
        size_t addr = current_pos;
        current_pos += align(s);
        if (size_current() > size_total()) {
            size_t snew = std::max(s+ALIGNMENT, 2*size);
            // TODO: save the old `start` and free it in ~Allocator()
            start = malloc(snew);
            if (start == nullptr) throw std::runtime_error("malloc failed.");
            current_pos = (size_t)start;
            current_pos = align(current_pos);
            size = snew;

            addr = current_pos;
            current_pos += align(s);

            LFORTRAN_ASSERT(size_current() <= size_total());
        }
        return (void*)addr;
    }

    // Allocates `n` elements of type T, returns the pointer T* to the first
    // element
    template <typename T> T* allocate(size_t n=1) {
        return (T *)alloc(sizeof(T) * n);
    }

    // Just like `new`, but using Allocator
    // The following two examples both construct the same instance MyInt(5),
    // but first uses the default C++ allocator, while the second uses
    // Allocator:
    //
    //     MyInt *n = new MyInt(5);          // Default C++ allocator
    //
    //     Allocator al(1024);
    //     MyInt *n = al.make_new<MyInt>(5); // Allocator
    template <typename T, typename... Args> T* make_new(Args &&... args) {
        return new(alloc(sizeof(T))) T(std::forward<Args>(args)...);
        // To test the default "new", comment the above and uncomment this:
        //return new T(std::forward<Args>(args)...);
    }

    size_t size_current() {
        return current_pos - (size_t)start;
    }

    size_t size_total() {
        return size;
    }
};

#endif
