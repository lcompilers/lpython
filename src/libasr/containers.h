#ifndef LFORTRAN_CONTAINERS_H
#define LFORTRAN_CONTAINERS_H

#include <cstring>
#include <libasr/alloc.h>

namespace LFortran
{

// Vector implementation

template <typename T>
struct Vec;

template <typename T>
class VecIterator
{
public:
    VecIterator(const Vec<T>& c, size_t idx=0)
        : m_container(c), m_index(idx) {}

    bool operator!=(const VecIterator& other) {
        return (m_index != other.m_index);
    }

    const VecIterator& operator++() {
        m_index++;
        return *this;
    }

    const T& operator*() const {
        return m_container[m_index];
    }
private:
    const Vec<T>& m_container;
    size_t m_index;
};

#ifdef WITH_LFORTRAN_ASSERT
static int vec_called_const = 0xdeadbeef;
#endif

template <typename T>
struct Vec {
    size_t n, max;
    T* p;
#ifdef WITH_LFORTRAN_ASSERT
    int reserve_called;
#endif

    // reserve() must be called before calling push_back()
    void reserve(Allocator &al, size_t max) {
        n = 0;
        if (max == 0) max++;
        LFORTRAN_ASSERT(max > 0)
        this->max = max;
        p = al.allocate<T>(max);
#ifdef WITH_LFORTRAN_ASSERT
        reserve_called = vec_called_const;
#endif
    }

    void push_back(Allocator &al, T x) {
        // This can pass by accident even if reserve() is not called (if
        // reserve_called happens to be equal to vec_called_const when Vec is
        // allocated in memory), but the chance is small. It catches such bugs
        // in practice.
        LFORTRAN_ASSERT(reserve_called == vec_called_const);
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

    void resize(Allocator &al, size_t max){
        reserve(al, max);
        n = max;
    }

    size_t capacity() const {
        return max;
    }

    // return a direct access to the underlying array
    T* data() const {
        return p;
    }

    const T& operator[](size_t pos) const {
        return p[pos];
    }

    // Returns a copy of the data as std::vector
    std::vector<T> as_vector() const {
        return std::vector<T>(p, p+n);
    }

    void from_pointer_n(T* p, size_t n) {
        this->p = p;
        this->n = n;
        this->max = n;
#ifdef WITH_LFORTRAN_ASSERT
        reserve_called = vec_called_const;
#endif
    }

    void from_pointer_n_copy(Allocator &al, T* p, size_t n) {
        this->reserve(al, n);
        for (size_t i=0; i<n; i++) {
            this->push_back(al, p[i]);
        }
    }

    VecIterator<T> begin() const {
        return VecIterator<T>(*this, 0);
    }

    VecIterator<T> end() const {
        return VecIterator<T>(*this, n);
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

    // Initializes Str from std::string by making a copy excluding the null char
    void from_str(Allocator &al, const std::string &s) {
        n = s.size();
        p = al.allocate<char>(n);
        std::memcpy(p, &s[0], sizeof(char) * n);
    }

    // Initializes Str from std::string by setting the pointer to point
    // to the std::string (no copy), and the length excluding the null char.
    // The original std::string cannot go out of scope if you are still using
    // Str. This function is helpful if you want to allocate a null terminated
    // C string using Allocator as follows:
    //
    //    std::string s
    //    ...
    //    Str a;
    //    a.from_str_view(s);
    //    char *s2 = a.c_str(al);
    void from_str_view(const std::string &s) {
        n = s.size();
        p = const_cast<char*>(&s[0]);
    }

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

template <typename ...Args>
std::string string_format(const std::string& format, Args && ...args)
{
    auto size = std::snprintf(nullptr, 0, format.c_str(), std::forward<Args>(args)...);
    std::string output(size, '\0');
    std::snprintf(&output[0], size + 1, format.c_str(), std::forward<Args>(args)...);
    return output;
}

static inline std::string double_to_scientific(double x) {
    return string_format("%25.17e", x);
}

} // namespace LFortran




#endif
