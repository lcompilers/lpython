#ifndef LFORTRAN_PARSER_ALLOC_H
#define LFORTRAN_PARSER_ALLOC_H

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
        if (start == nullptr) {
            throw std::runtime_error("malloc failed.");
        }
        current_pos = (size_t)start;
        current_pos = align(current_pos);
        size = s;
    }

    void *allocate(size_t s) {
        if (start == nullptr) {
            throw std::bad_alloc();
        }
        size_t addr = current_pos;
        /*
        std::cout << "start:" << (size_t)start << std::endl;
        std::cout << "addr:" << addr << std::endl;
        std::cout << "size:" << size << std::endl;
        std::cout << "s:" << s << std::endl;
        std::cout << "align(s):" << align(s) << std::endl;
        */
        current_pos += align(s);
        if (current_pos - (size_t)start > size) {
            //throw std::runtime_error("Linear allocator too small.");
            throw std::bad_alloc();
        }
        return (void*)addr;
    }

    template <typename T, typename... Args> T* make_new(Args &&... args) {
        return new(allocate(sizeof(T))) T(std::forward<Args>(args)...);
        // To test the default "new", comment the above and uncomment this:
        //return new T(std::forward<Args>(args)...);
    }
};

#endif
