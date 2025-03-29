#ifndef LFORTRAN_CONTAINERS_H
#define LFORTRAN_CONTAINERS_H

#include <cstring>
#include <libasr/alloc.h>

namespace LCompilers {

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
        LCOMPILERS_ASSERT(max > 0)
        this->max = max;
        p = al.allocate<T>(max);
#ifdef WITH_LFORTRAN_ASSERT
        reserve_called = vec_called_const;
#endif
    }

    template <class Q = T>
    typename std::enable_if<std::is_same<Q, char*>::value, bool>::type present(Q x, size_t& index) {
        for( size_t i = 0; i < n; i++ ) {
            if( strcmp(p[i], x) == 0 ) {
                index = i;
                return true;
            }
        }
        return false;
    }

    template <class Q = T>
    typename std::enable_if<!std::is_same<Q, char*>::value, bool>::type present(Q x, size_t& index) {
        for( size_t i = 0; i < n; i++ ) {
            if( p[i] == x ) {
                index = i;
                return true;
            }
        }
        return false;
    }

    void erase(T x) {
        size_t delete_index;
        if( !present(x, delete_index) ) {
            return ;
        }

        for( int64_t i = delete_index; i < (int64_t) n - 1; i++ ) {
            p[i] = p[i + 1];
        }
        if( n >= 1 ) {
            n = n - 1;
        }
    }

    void push_back_unique(Allocator &al, T x) {
        size_t index;
        if( !Vec<T>::present(x, index) ) {
            Vec<T>::push_back(al, x);
        }
    }

    void push_back(Allocator &al, T x) {
        // This can pass by accident even if reserve() is not called (if
        // reserve_called happens to be equal to vec_called_const when Vec is
        // allocated in memory), but the chance is small. It catches such bugs
        // in practice.
        LCOMPILERS_ASSERT(reserve_called == vec_called_const);
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

    bool empty() const {
        return n == 0;
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

    T& back() const {
        return p[n - 1];
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

/*
SetChar emulates the std::set<std::string> API
so that it acts as a drop in replacement.
*/
struct SetChar: Vec<char*> {

    bool reserved;

    SetChar():
        reserved(false) {
        clear();
    }

    void clear() {
        n = 0;
        p = nullptr;
        max = 0;
    }

    void clear(Allocator& al) {
        reserve(al, 0);
    }

    void reserve(Allocator& al, size_t max) {
        Vec<char*>::reserve(al, max);
        reserved = true;
    }

    void from_pointer_n_copy(Allocator &al, char** p, size_t n) {
        reserve(al, n);
        for (size_t i = 0; i < n; i++) {
            push_back(al, p[i]);
        }
    }

    void from_pointer_n(char** p, size_t n) {
        Vec<char*>::from_pointer_n(p, n);
        reserved = true;
    }

    void push_back(Allocator &al, char* x) {
         if( !reserved ) {
             reserve(al, 0);
         }

         Vec<char*>::push_back_unique(al, x);
     }
};

// String implementation (not null-terminated)
struct Str {
    size_t n;
    char* p;

    // Returns a copy of the string as a NULL terminated std::string
    std::string str() const { return std::string(p, n); }

    char operator[](size_t pos) {
        return p[pos];
    }

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

    char back() const {
        return p[n - 1];
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

} // namespace LCompilers




#endif
