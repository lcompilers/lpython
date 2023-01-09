#ifndef LFORTRAN_BIGINT_H
#define LFORTRAN_BIGINT_H

#include <cstdint>

#include <libasr/containers.h>

namespace LFortran {

namespace BigInt {

/*
 * Arbitrary size integer implementation.
 *
 * We use tagged signed 64bit integers with no padding bits and using 2's
 * complement for negative values (int64_t) as the underlying data structure.
 * Little-endian is assumed.
 *
 * Bits (from the left):
 * 1 ..... sign: 0 positive, 1 negative
 * 2 ..... tag: bits 1-2 equal to 01: pointer; otherwise integer
 * 3-64 .. if the tag is - integer: rest of the signed integer bits in 2's
 *                                  complement
 *                       - pointer: 64 bit pointer shifted by 2
 *                                  to the right (>> 2)
 *
 * The pointer must be aligned to 4 bytes (bits 63-64 must be 00).
 * Small signed integers are represented directly as integers in int64_t, large
 * integers are allocated on heap and a pointer to it is used as "tag pointer"
 * in int64_t.
 *
 * To check if the integer has a pointer tag, we check that the first two bits
 * (1-2) are equal to 01.
 *
 * If the first bit is 0, then it can either be a positive integer or a
 * pointer. We check the second bit, if it is 1, then it is a pointer (shifted
 * by 2), if it is 0, then is is a positive integer, represented by the rest of
 * the 62 bits. If the first bit is 1, then it is a negative integer,
 * represented by the full 64 bits in 2's complement representation.
 */

// Returns true if "i" is a pointer and false if "i" is an integer
inline static bool is_int_ptr(int64_t i) {
    return (((uint64_t)i) >> (64 - 2)) == 1;
}

/*
 * A pointer is converted to integer by shifting by 2 to the right and adding
 * 01 to the first two bits to tag it as a pointer:
 */

// Converts a pointer "p" (must be aligned to 4 bytes) to a tagged int64_t
inline static int64_t ptr_to_int(void *p) {
    return (int64_t)( (((uint64_t)p) >> 2) | (1ULL << (64 - 2)) );
}

/* An integer with the pointer tag is converted to a pointer by shifting by 2
 * to the left, which erases the tag and puts 00 to bits 63-64:
 */

// Converts a tagged int64_t to a pointer (aligned to 4 bytes)
inline static void* int_to_ptr(int64_t i) {
    return (void *)(((uint64_t)i) << 2);
}

/* The maximum small int is 2^62-1
 */
const int64_t MAX_SMALL_INT = (int64_t)((1ULL << 62)-1);

/* The minimum small int is -2^63
 */
const int64_t MIN_SMALL_INT = (int64_t)(-(1ULL << 63));

// Returns true if "i" is a small int
inline static bool is_small_int(int64_t i) {
    return (MIN_SMALL_INT <= i && i <= MAX_SMALL_INT);
}

/* Arbitrary integer implementation
 * For now large integers are implemented as strings with decimal digits. The
 * only supported operation on this is converting to and from a string. Later
 * we will replace with an actual large integer implementation and add other
 * operations.
 */

// Converts a string to a large int (allocated on heap, returns a pointer)
inline static int64_t string_to_largeint(Allocator &al, const Str &s) {
    char *cs = s.c_str(al);
    return ptr_to_int(cs);
}

// Converts a large int to a string
inline static char* largeint_to_string(int64_t i) {
    LFORTRAN_ASSERT(is_int_ptr(i));
    void *p = int_to_ptr(i);
    char *cs = (char*)p;
    return cs;
}

inline static std::string int_to_str(int64_t i) {
    if (is_int_ptr(i)) {
        return std::string(largeint_to_string(i));
    } else {
        return std::to_string(i);
    }
}

inline static bool is_int64(std::string str_repr) {
    std::string str_int64 = "9223372036854775807";
    if( str_repr.size() > str_int64.size() ) {
        return false;
    }

    if( str_repr.size() < str_int64.size() ) {
        return true;
    }

    size_t i;
    for( i = 0; i < str_repr.size() - 1 && str_repr[i] == str_int64[i]; i++  ) {
    }
    return i == str_repr.size() - 1 || str_repr[i] < str_int64[i];
}

/* BigInt is a thin wrapper over the functionality exposed in the functions
 * above.  The idea is that one can use the int64_t type directly and just use
 * the function above to handle the large integer aspects, and if it is a small
 * integer, one can use it directly as int64 integer.
 *
 * Alternatively, one can use the BigInt class below that exposes the
 * functionality via methods.
 */

struct BigInt {
    int64_t n;

    BigInt() = default;
    BigInt(const BigInt &) = default;
    BigInt& operator=(const BigInt &) = default;

    void from_smallint(int64_t i) {
        LFORTRAN_ASSERT(is_small_int(i));
        n = i;
    }

    void from_largeint(Allocator &al, const Str &s) {
        n = string_to_largeint(al, s);
    }

    bool is_large() const {
        return is_int_ptr(n);
    }

    int64_t as_smallint() const {
        LFORTRAN_ASSERT(!is_large());
        return n;
    }

    std::string str() const {
        return int_to_str(n);
    }

};

static_assert(std::is_standard_layout<BigInt>::value);
static_assert(std::is_trivial<BigInt>::value);
static_assert(sizeof(BigInt) == sizeof(int64_t));
static_assert(sizeof(BigInt) == 8);


} // BigInt

} // LFortran

#endif // LFORTRAN_BIGINT_H
