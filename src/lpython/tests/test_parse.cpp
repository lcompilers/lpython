#include <tests/doctest.h>

#include <iostream>
#include <sstream>
#include <chrono>
#include <string>

#include <lpython/bigint.h>

using LCompilers::TRY;
using LCompilers::Result;

using LCompilers::LPython::BigInt::is_int_ptr;
using LCompilers::LPython::BigInt::ptr_to_int;
using LCompilers::LPython::BigInt::int_to_ptr;
using LCompilers::LPython::BigInt::string_to_largeint;
using LCompilers::LPython::BigInt::largeint_to_string;
using LCompilers::LPython::BigInt::MAX_SMALL_INT;
using LCompilers::LPython::BigInt::MIN_SMALL_INT;

// Print any vector like iterable to a string
template <class T>
inline std::ostream &print_vec(std::ostream &out, T &d)
{
    out << "[";
    for (auto p = d.begin(); p != d.end(); p++) {
        if (p != d.begin())
            out << ", ";
        out << *p;
    }
    out << "]";
    return out;
}


namespace doctest {
    // Convert std::vector<T> to string for doctest
    template<typename T> struct StringMaker<std::vector<T>> {
        static String convert(const std::vector<T> &value) {
            std::ostringstream oss;
            print_vec(oss, value);
            return oss.str().c_str();
        }
    };
}


class TokenizerError0 {
};


TEST_CASE("Test Big Int") {
    int64_t i;
    void *p, *p2;

    /* Integer tests */
    i = 0;
    CHECK(!is_int_ptr(i));

    i = 5;
    CHECK(!is_int_ptr(i));

    i = -5;
    CHECK(!is_int_ptr(i));

    // Largest integer that is allowed is 2^62-1
    i = 4611686018427387903LL;
    CHECK(i == MAX_SMALL_INT);
    CHECK(!is_int_ptr(i)); // this is an integer
    i = 4611686018427387904LL;
    CHECK(is_int_ptr(i)); // this is a pointer

    // Smallest integer that is allowed is -2^63
    i = -9223372036854775808ULL;
    CHECK(i == MIN_SMALL_INT);
    CHECK(!is_int_ptr(i)); // this is an integer
    i = -9223372036854775809ULL; // This does not fit into a signed 64bit int
    CHECK(is_int_ptr(i)); // this is a pointer

    /* Pointer tests */
    // Smallest pointer value is 0 (nullptr)
    p = nullptr;
    i = ptr_to_int(p);
    CHECK(is_int_ptr(i));
    p2 = int_to_ptr(i);
    CHECK(p == p2);

    // Second smallest pointer value aligned to 4 is 4
    p = (void*)4;
    i = ptr_to_int(p);
    CHECK(is_int_ptr(i));
    p2 = int_to_ptr(i);
    CHECK(p == p2);

    // Maximum pointer value aligned to 4 is (2^64-1)-3
    p = (void*)18446744073709551612ULL;
    i = ptr_to_int(p);
    CHECK(is_int_ptr(i));
    p2 = int_to_ptr(i);
    CHECK(p == p2);

    /* Big int tests */
    Allocator al(1024);
    LCompilers::Str s;
    char *cs;

    s.from_str(al, "123");
    i = string_to_largeint(al, s);
    CHECK(is_int_ptr(i));
    cs = largeint_to_string(i);
    CHECK(std::string(cs) == "123");

    s.from_str(al, "123567890123456789012345678901234567890");
    i = string_to_largeint(al, s);
    CHECK(is_int_ptr(i));
    cs = largeint_to_string(i);
    CHECK(std::string(cs) == "123567890123456789012345678901234567890");
}

TEST_CASE("Test LCompilers::Vec") {
    Allocator al(1024);
    LCompilers::Vec<int> v;

    v.reserve(al, 2);
    CHECK(v.size() == 0);
    CHECK(v.capacity() == 2);

    v.push_back(al, 1);
    CHECK(v.size() == 1);
    CHECK(v.capacity() == 2);
    CHECK(v.p[0] == 1);
    CHECK(v[0] == 1);

    v.push_back(al, 2);
    CHECK(v.size() == 2);
    CHECK(v.capacity() == 2);
    CHECK(v.p[0] == 1);
    CHECK(v.p[1] == 2);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);

    v.push_back(al, 3);
    CHECK(v.size() == 3);
    CHECK(v.capacity() == 4);
    CHECK(v.p[0] == 1);
    CHECK(v.p[1] == 2);
    CHECK(v.p[2] == 3);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 3);

    v.push_back(al, 4);
    CHECK(v.size() == 4);
    CHECK(v.capacity() == 4);
    CHECK(v.p[0] == 1);
    CHECK(v.p[1] == 2);
    CHECK(v.p[2] == 3);
    CHECK(v.p[3] == 4);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 3);
    CHECK(v[3] == 4);

    v.push_back(al, 5);
    CHECK(v.size() == 5);
    CHECK(v.capacity() == 8);
    CHECK(v.p[0] == 1);
    CHECK(v.p[1] == 2);
    CHECK(v.p[2] == 3);
    CHECK(v.p[3] == 4);
    CHECK(v.p[4] == 5);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 3);
    CHECK(v[3] == 4);
    CHECK(v[4] == 5);


    std::vector<int> sv = v.as_vector();
    CHECK(sv.size() == 5);
    CHECK(&(sv[0]) != &(v.p[0]));
    CHECK(sv[0] == 1);
    CHECK(sv[1] == 2);
    CHECK(sv[2] == 3);
    CHECK(sv[3] == 4);
    CHECK(sv[4] == 5);
}

TEST_CASE("LCompilers::Vec iterators") {
    Allocator al(1024);
    LCompilers::Vec<int> v;

    v.reserve(al, 2);
    v.push_back(al, 1);
    v.push_back(al, 2);
    v.push_back(al, 3);
    v.push_back(al, 4);

    // Check reference (auto)
    int i = 0;
    for (auto &a : v) {
        i += a;
    }
    CHECK(i == 10);

    // Check reference (must be const)
    i = 0;
    for (const int &a : v) {
        i += a;
    }
    CHECK(i == 10);

    // Check direct type (auto)
    i = 0;
    for (auto a : v) {
        i += a;
    }
    CHECK(i == 10);

    // Check direct type (const)
    i = 0;
    for (const int a : v) {
        i += a;
    }
    CHECK(i == 10);

    // Check direct type (non const)
    i = 0;
    for (int a : v) {
        i += a;
    }
    CHECK(i == 10);
}

TEST_CASE("Test LCompilers::Str") {
    Allocator al(1024);
    LCompilers::Str s;
    const char *data = "Some string.";

    s.p = const_cast<char*>(data);
    s.n = 2;
    CHECK(s.size() == 2);
    CHECK(s.p == data);
    CHECK(s.str() == "So");

    std::string scopy = s.str();
    CHECK(s.p != &scopy[0]);
    CHECK(scopy == "So");
    CHECK(scopy[0] == 'S');
    CHECK(scopy[1] == 'o');
    CHECK(scopy[2] == '\x00');

    char *copy = s.c_str(al);
    CHECK(s.p != copy);
    CHECK(copy[0] == 'S');
    CHECK(copy[1] == 'o');
    CHECK(copy[2] == '\x00');
}

TEST_CASE("Test LCompilers::Allocator") {
    Allocator al(32);
    // Size is what we asked (32) plus alignment (8) = 40
    CHECK(al.size_total() == 40);

    // Fits in the pre-allocated chunk
    al.alloc(32);
    CHECK(al.size_total() == 40);

    // Chunk doubles
    al.alloc(32);
    CHECK(al.size_total() == 80);

    // Chunk doubles
    al.alloc(90);
    CHECK(al.size_total() == 160);

    // We asked more than can fit in the doubled chunk (2*160),
    // so the chunk will be equal to what we asked (1024) plus alignment (8)
    al.alloc(1024);
    CHECK(al.size_total() == 1032);
}

TEST_CASE("Test LCompilers::Allocator 2") {
    Allocator al(32);
    int *p = al.allocate<int>();
    p[0] = 5;

    p = al.allocate<int>(3);
    p[0] = 1;
    p[1] = 2;
    p[2] = 3;

    std::vector<int> *v = al.make_new<std::vector<int>>(5);
    CHECK(v->size() == 5);
    // Must manually call the destructor:
    v->~vector<int>();
}
