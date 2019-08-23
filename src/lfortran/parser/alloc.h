#ifndef LFORTRAN_PARSER_ALLOC_H
#define LFORTRAN_PARSER_ALLOC_H

#include <stdexcept>
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
        start = malloc(s);
        if (start == nullptr) throw std::runtime_error("malloc failed.");
        current_pos = (size_t)start;
        current_pos = align(current_pos);
        size = s;
    }
    ~Allocator() {
        if (start != nullptr) free(start);
    }

    void *allocate(size_t s) {
        LFORTRAN_ASSERT(start != nullptr);
        size_t addr = current_pos;
        current_pos += align(s);
        if (size_current() > size_total()) throw std::bad_alloc();
        return (void*)addr;
    }

    template <typename T> T* alloc(size_t n) {
        return (T *)allocate(sizeof(T) * n);
    }

    template <typename T, typename... Args> T* make_new(Args &&... args) {
        return new(allocate(sizeof(T))) T(std::forward<Args>(args)...);
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
