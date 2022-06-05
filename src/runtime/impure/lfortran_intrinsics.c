#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <float.h>
#include <limits.h>

#include "lfortran_intrinsics.h"


LCOMPILERS_API double _lcompilers_sum(int n, double *v)
{
    int i, r;
    r = 0;
    for (i=0; i < n; i++) {
        r += v[i];
    }
    return r;
}

LCOMPILERS_API void _lcompilers_random_number(int n, double *v)
{
    int i;
    for (i=0; i < n; i++) {
        v[i] = rand() / (double) RAND_MAX;
    }
}

LCOMPILERS_API double _lcompilers_random()
{
    return (rand() / (double) RAND_MAX);
}

LCOMPILERS_API int _lcompilers_randrange(int lower, int upper)
{
    int rr = lower + (rand() % (upper - lower));
    return rr;
}

LCOMPILERS_API int _lcompilers_random_int(int lower, int upper)
{
    int randint = lower + (rand() % (upper - lower + 1));
    return randint;
}

LCOMPILERS_API void _lcompilers_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    fflush(stdout);
    va_end(args);
}

LCOMPILERS_API void _lcompilers_complex_add_32(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32 *result)
{
    result->re = a->re + b->re;
    result->im = a->im + b->im;
}

LCOMPILERS_API void _lcompilers_complex_add_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result)
{
    result->re = a->re + b->re;
    result->im = a->im + b->im;
}

LCOMPILERS_API void _lcompilers_complex_sub_32(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32 *result)
{
    result->re = a->re - b->re;
    result->im = a->im - b->im;
}

LCOMPILERS_API void _lcompilers_complex_sub_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result)
{
    result->re = a->re - b->re;
    result->im = a->im - b->im;
}

LCOMPILERS_API void _lcompilers_complex_mul_32(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32 *result)
{
    float p = a->re, q = a->im;
    float r = b->re, s = b->im;
    result->re = (p*r - q*s);
    result->im = (p*s + q*r);
}

LCOMPILERS_API void _lcompilers_complex_mul_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result)
{
    double p = a->re, q = a->im;
    double r = b->re, s = b->im;
    result->re = (p*r - q*s);
    result->im = (p*s + q*r);
}

LCOMPILERS_API void _lcompilers_complex_div_32(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32 *result)
{
    float p = a->re, q = a->im;
    float r = b->re, s = -(b->im);
    float mod_b = r*r + s*s;
    result->re = (p*r - q*s)/mod_b;
    result->im = (p*s + q*r)/mod_b;
}

LCOMPILERS_API void _lcompilers_complex_div_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result)
{
    double p = a->re, q = a->im;
    double r = b->re, s = -(b->im);
    double mod_b = r*r + s*s;
    result->re = (p*r - q*s)/mod_b;
    result->im = (p*s + q*r)/mod_b;
}

#undef CMPLX
#undef CMPLXF
#undef CMPLXL
#undef _Imaginary_I

#define _Imaginary_I (I)
#define CMPLX(x, y) ((double complex)((double)(x) + _Imaginary_I * (double)(y)))
#define CMPLXF(x, y) ((float complex)((float)(x) + _Imaginary_I * (float)(y)))
#define CMPLXL(x, y) ((long double complex)((long double)(x) + \
                      _Imaginary_I * (long double)(y)))
#define BITS_32 32
#define BITS_64 64

LCOMPILERS_API void _lcompilers_complex_pow_32(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32 *result)
{
    #ifdef _MSC_VER
        _Fcomplex ca = _FCOMPLEX_(a->re, a->im);
        _Fcomplex cb = _FCOMPLEX_(b->re, b->im);
        _Fcomplex cr = cpowf(ca, cb);
    #else
        float complex ca = CMPLXF(a->re, a->im);
        float complex cb = CMPLXF(b->re, b->im);
        float complex cr = cpowf(ca, cb);
    #endif
        result->re = crealf(cr);
        result->im = cimagf(cr);

}

LCOMPILERS_API void _lcompilers_complex_pow_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result)
{
    #ifdef _MSC_VER
        _Dcomplex ca = _DCOMPLEX_(a->re, a->im);
        _Dcomplex cb = _DCOMPLEX_(b->re, b->im);
        _Dcomplex cr = cpow(ca, cb);
    #else
        double complex ca = CMPLX(a->re, a->im);
        double complex cb = CMPLX(b->re, b->im);
        double complex cr = cpow(ca, cb);
    #endif
        result->re = creal(cr);
        result->im = cimag(cr);

}

// sqrt ------------------------------------------------------------------------

LCOMPILERS_API float_complex_t _lcompilers_csqrt(float_complex_t x)
{
    return csqrtf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zsqrt(double_complex_t x)
{
    return csqrt(x);
}

// aimag -----------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_caimag(float_complex_t x)
{
    return cimagf(x);
}

LCOMPILERS_API double _lcompilers_zaimag(double_complex_t x)
{
    return cimag(x);
}

LCOMPILERS_API void _lcompilers_complex_aimag_32(struct _lcompilers_complex_32 *x, float *res)
{
    *res = x->im;
}

LCOMPILERS_API void _lcompilers_complex_aimag_64(struct _lcompilers_complex_64 *x, double *res)
{
    *res = x->im;
}

// exp -------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_sexp(float x)
{
    return expf(x);
}

LCOMPILERS_API double _lcompilers_dexp(double x)
{
    return exp(x);
}

LCOMPILERS_API float_complex_t _lcompilers_cexp(float_complex_t x)
{
    return cexpf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zexp(double_complex_t x)
{
    return cexp(x);
}

// log -------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_slog(float x)
{
    return logf(x);
}

LCOMPILERS_API double _lcompilers_dlog(double x)
{
    return log(x);
}

LCOMPILERS_API float_complex_t _lcompilers_clog(float_complex_t x)
{
    return clogf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zlog(double_complex_t x)
{
    return clog(x);
}

// erf -------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_serf(float x)
{
    return erff(x);
}

LCOMPILERS_API double _lcompilers_derf(double x)
{
    return erf(x);
}

// erfc ------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_serfc(float x)
{
    return erfcf(x);
}

LCOMPILERS_API double _lcompilers_derfc(double x)
{
    return erfc(x);
}

// log10 -----------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_slog10(float x)
{
    return log10f(x);
}

LCOMPILERS_API double _lcompilers_dlog10(double x)
{
    return log10(x);
}

// gamma -----------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_sgamma(float x)
{
    return tgammaf(x);
}

LCOMPILERS_API double _lcompilers_dgamma(double x)
{
    return tgamma(x);
}

// gamma -----------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_slog_gamma(float x)
{
    return lgammaf(x);
}

LCOMPILERS_API double _lcompilers_dlog_gamma(double x)
{
    return lgamma(x);
}

// sin -------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_ssin(float x)
{
    return sinf(x);
}

LCOMPILERS_API double _lcompilers_dsin(double x)
{
    return sin(x);
}

LCOMPILERS_API float_complex_t _lcompilers_csin(float_complex_t x)
{
    return csinf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zsin(double_complex_t x)
{
    return csin(x);
}

// cos -------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_scos(float x)
{
    return cosf(x);
}

LCOMPILERS_API double _lcompilers_dcos(double x)
{
    return cos(x);
}

LCOMPILERS_API float_complex_t _lcompilers_ccos(float_complex_t x)
{
    return ccosf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zcos(double_complex_t x)
{
    return ccos(x);
}

// tan -------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_stan(float x)
{
    return tanf(x);
}

LCOMPILERS_API double _lcompilers_dtan(double x)
{
    return tan(x);
}

LCOMPILERS_API float_complex_t _lcompilers_ctan(float_complex_t x)
{
    return ctanf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_ztan(double_complex_t x)
{
    return ctan(x);
}

// sinh ------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_ssinh(float x)
{
    return sinhf(x);
}

LCOMPILERS_API double _lcompilers_dsinh(double x)
{
    return sinh(x);
}

LCOMPILERS_API float_complex_t _lcompilers_csinh(float_complex_t x)
{
    return csinhf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zsinh(double_complex_t x)
{
    return csinh(x);
}

// cosh ------------------------------------------------------------------------


LCOMPILERS_API float _lcompilers_scosh(float x)
{
    return coshf(x);
}

LCOMPILERS_API double _lcompilers_dcosh(double x)
{
    return cosh(x);
}

LCOMPILERS_API float_complex_t _lcompilers_ccosh(float_complex_t x)
{
    return ccoshf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zcosh(double_complex_t x)
{
    return ccosh(x);
}

// tanh ------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_stanh(float x)
{
    return tanhf(x);
}

LCOMPILERS_API double _lcompilers_dtanh(double x)
{
    return tanh(x);
}

LCOMPILERS_API float_complex_t _lcompilers_ctanh(float_complex_t x)
{
    return ctanhf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_ztanh(double_complex_t x)
{
    return ctanh(x);
}

// asin ------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_sasin(float x)
{
    return asinf(x);
}

LCOMPILERS_API double _lcompilers_dasin(double x)
{
    return asin(x);
}

LCOMPILERS_API float_complex_t _lcompilers_casin(float_complex_t x)
{
    return casinf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zasin(double_complex_t x)
{
    return casin(x);
}

// acos ------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_sacos(float x)
{
    return acosf(x);
}

LCOMPILERS_API double _lcompilers_dacos(double x)
{
    return acos(x);
}

LCOMPILERS_API float_complex_t _lcompilers_cacos(float_complex_t x)
{
    return cacosf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zacos(double_complex_t x)
{
    return cacos(x);
}

// atan ------------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_satan(float x)
{
    return atanf(x);
}

LCOMPILERS_API double _lcompilers_datan(double x)
{
    return atan(x);
}

LCOMPILERS_API float_complex_t _lcompilers_catan(float_complex_t x)
{
    return catanf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zatan(double_complex_t x)
{
    return catan(x);
}

// atan2 -----------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_satan2(float y, float x)
{
    return atan2f(y, x);
}

LCOMPILERS_API double _lcompilers_datan2(double y, double x)
{
    return atan2(y, x);
}

// asinh -----------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_sasinh(float x)
{
    return asinhf(x);
}

LCOMPILERS_API double _lcompilers_dasinh(double x)
{
    return asinh(x);
}

LCOMPILERS_API float_complex_t _lcompilers_casinh(float_complex_t x)
{
    return casinhf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zasinh(double_complex_t x)
{
    return casinh(x);
}

// acosh -----------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_sacosh(float x)
{
    return acoshf(x);
}

LCOMPILERS_API double _lcompilers_dacosh(double x)
{
    return acosh(x);
}

LCOMPILERS_API float_complex_t _lcompilers_cacosh(float_complex_t x)
{
    return cacoshf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zacosh(double_complex_t x)
{
    return cacosh(x);
}

// fmod -----------------------------------------------------------------------

LCOMPILERS_API double _lcompilers_dfmod(double x, double y)
{
    return fmod(x, y);
}

// atanh -----------------------------------------------------------------------

LCOMPILERS_API float _lcompilers_satanh(float x)
{
    return atanhf(x);
}

LCOMPILERS_API double _lcompilers_datanh(double x)
{
    return atanh(x);
}

LCOMPILERS_API float_complex_t _lcompilers_catanh(float_complex_t x)
{
    return catanhf(x);
}

LCOMPILERS_API double_complex_t _lcompilers_zatanh(double_complex_t x)
{
    return catanh(x);
}


// strcat  --------------------------------------------------------------------

LCOMPILERS_API void _lcompilers_strcat(char** s1, char** s2, char** dest)
{
    int cntr = 0;
    char trmn = '\0';
    int s1_len = strlen(*s1);
    int s2_len = strlen(*s2);
    int trmn_size = sizeof(trmn);
    char* dest_char = (char*)malloc(s1_len+s2_len+trmn_size);
    for (int i = 0; i < s1_len; i++) {
        dest_char[cntr] = (*s1)[i];
        cntr++;
    }
    for (int i = 0; i < s2_len; i++) {
        dest_char[cntr] = (*s2)[i];
        cntr++;
    }
    dest_char[cntr] = trmn;
    *dest = &(dest_char[0]);
}

//repeat str for n time
LCOMPILERS_API void _lcompilers_strrepeat(char** s, int32_t n, char** dest)
{
    int cntr = 0;
    char trmn = '\0';
    int s_len = strlen(*s);
    int trmn_size = sizeof(trmn);
    int f_len = s_len*n;
    if (f_len < 0)
        f_len = 0;
    char* dest_char = (char*)malloc(f_len+trmn_size);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < s_len; j++) {
            dest_char[cntr] = (*s)[j];
            cntr++;
        }
    }
    dest_char[cntr] = trmn;
    *dest = &(dest_char[0]);
}

// idx1 and idx2 both start from 1
LCOMPILERS_API char* _lcompilers_str_copy(char* s, int32_t idx1, int32_t idx2) {
    int s_len = strlen(s);
    if(idx1 > s_len || idx1 <= (-1*s_len)){
        printf("String index out of Bounds\n");
        exit(1);
    }
    if(idx1 <= 0) {
        idx1 = s_len + idx1;
    }
    if(idx2 <= 0) {
        idx2 = s_len + idx2;
    }
    char* dest_char = (char*)malloc(idx2-idx1+2);
    for (int i=idx1; i <= idx2; i++)
    {
        dest_char[i-idx1] = s[i-1];
    }
    dest_char[idx2-idx1+1] = '\0';
    return dest_char;
}

LCOMPILERS_API int _lcompilers_str_len(char** s)
{
    return strlen(*s);
}

LCOMPILERS_API char* _lcompilers_malloc(int size) {
    return (char*)malloc(size);
}

LCOMPILERS_API void _lcompilers_free(char* ptr) {
    free((void*)ptr);
}

// size_plus_one is the size of the string including the null character
LCOMPILERS_API void _lcompilers_string_init(int size_plus_one, char *s) {
    int size = size_plus_one-1;
    for (int i=0; i < size; i++) {
        s[i] = ' ';
    }
    s[size] = '\0';
}

// List  -----------------------------------------------------------------------

struct _lcompilers_list_i32 {
    uint64_t n;
    uint64_t capacity;
    int32_t *p;
};

LCOMPILERS_API int8_t* _lcompilers_list_init_i32() {
    struct _lcompilers_list_i32 *l;
    l = (struct _lcompilers_list_i32*)malloc(
            sizeof(struct _lcompilers_list_i32));
    l->n = 0;
    l->capacity = 4;
    l->p = (int32_t*)malloc(l->capacity*sizeof(int32_t));
    return (int8_t*)l;
}

LCOMPILERS_API void _lcompilers_list_append_i32(int8_t* s, int32_t item) {
    struct _lcompilers_list_i32 *l = (struct _lcompilers_list_i32 *)s;
    if (l->n == l->capacity) {
        l->capacity = 2*l->capacity;
        l->p = realloc(l->p, sizeof(int32_t)*l->capacity);
    }
    l->p[l->n] = item;
    l->n++;
}

// pos is the index = 1..n
LCOMPILERS_API int32_t _lcompilers_list_item_i32(int8_t* s, int32_t pos) {
    struct _lcompilers_list_i32 *l = (struct _lcompilers_list_i32 *)s;
    if (pos >= 1 && pos <= l->n) {
        return l->p[pos-1];
    } else {
        printf("Out of bounds\n");
        return 0;
    }
}

// bit  ------------------------------------------------------------------------

LCOMPILERS_API int32_t _lcompilers_iand32(int32_t x, int32_t y) {
    return x & y;
}

LCOMPILERS_API int64_t _lcompilers_iand64(int64_t x, int64_t y) {
    return x & y;
}

LCOMPILERS_API int32_t _lcompilers_not32(int32_t x) {
    return ~ x;
}

LCOMPILERS_API int64_t _lcompilers_not64(int64_t x) {
    return ~ x;
}

LCOMPILERS_API int32_t _lcompilers_ior32(int32_t x, int32_t y) {
    return x | y;
}

LCOMPILERS_API int64_t _lcompilers_ior64(int64_t x, int64_t y) {
    return x | y;
}

LCOMPILERS_API int32_t _lcompilers_ieor32(int32_t x, int32_t y) {
    return x ^ y;
}

LCOMPILERS_API int64_t _lcompilers_ieor64(int64_t x, int64_t y) {
    return x ^ y;
}

LCOMPILERS_API int32_t _lcompilers_ibclr32(int32_t i, int pos) {
    return i & ~(1 << pos);
}

LCOMPILERS_API int64_t _lcompilers_ibclr64(int64_t i, int pos) {
    return i & ~(1LL << pos);
}

LCOMPILERS_API int32_t _lcompilers_ibset32(int32_t i, int pos) {
    return i | (1 << pos);
}

LCOMPILERS_API int64_t _lcompilers_ibset64(int64_t i, int pos) {
    return i | (1LL << pos);
}

LCOMPILERS_API int32_t _lcompilers_btest32(int32_t i, int pos) {
    return i & (1 << pos);
}

LCOMPILERS_API int64_t _lcompilers_btest64(int64_t i, int pos) {
    return i & (1LL << pos);
}

LCOMPILERS_API int32_t _lcompilers_ishft32(int32_t i, int32_t shift) {
    if(shift > 0) {
        return i << shift;
    } else if(shift < 0) {
        return i >> abs(shift);
    } else {
        return i;
    }
}

LCOMPILERS_API int64_t _lcompilers_ishft64(int64_t i, int64_t shift) {
    if(shift > 0) {
        return i << shift;
    } else if(shift < 0) {
        return i >> llabs(shift);
    } else {
        return i;
    }
}

LCOMPILERS_API int32_t _lcompilers_mvbits32(int32_t from, int32_t frompos,
                                        int32_t len, int32_t to, int32_t topos) {
    uint32_t all_ones = ~0;
    uint32_t ufrom = from;
    uint32_t uto = to;
    all_ones <<= (BITS_32 - frompos - len);
    all_ones >>= (BITS_32 - len);
    all_ones <<= topos;
    ufrom <<= (BITS_32 - frompos - len);
    ufrom >>= (BITS_32 - len);
    ufrom <<= topos;
    return (~all_ones & uto) | ufrom;
}

LCOMPILERS_API int64_t _lcompilers_mvbits64(int64_t from, int32_t frompos,
                                        int32_t len, int64_t to, int32_t topos) {
    uint64_t all_ones = ~0;
    uint64_t ufrom = from;
    uint64_t uto = to;
    all_ones <<= (BITS_64 - frompos - len);
    all_ones >>= (BITS_64 - len);
    all_ones <<= topos;
    ufrom <<= (BITS_64 - frompos - len);
    ufrom >>= (BITS_64 - len);
    ufrom <<= topos;
    return (~all_ones & uto) | ufrom;
}

LCOMPILERS_API int32_t _lcompilers_bgt32(int32_t i, int32_t j) {
    uint32_t ui = i, uj = j;
    return ui > uj;
}

LCOMPILERS_API int32_t _lcompilers_bgt64(int64_t i, int64_t j) {
    uint64_t ui = i, uj = j;
    return ui > uj;
}

LCOMPILERS_API int32_t _lcompilers_bge32(int32_t i, int32_t j) {
    uint32_t ui = i, uj = j;
    return ui >= uj;
}

LCOMPILERS_API int32_t _lcompilers_bge64(int64_t i, int64_t j) {
    uint64_t ui = i, uj = j;
    return ui >= uj;
}

LCOMPILERS_API int32_t _lcompilers_ble32(int32_t i, int32_t j) {
    uint32_t ui = i, uj = j;
    return ui <= uj;
}

LCOMPILERS_API int32_t _lcompilers_ble64(int64_t i, int64_t j) {
    uint64_t ui = i, uj = j;
    return ui <= uj;
}

LCOMPILERS_API int32_t _lcompilers_blt32(int32_t i, int32_t j) {
    uint32_t ui = i, uj = j;
    return ui < uj;
}

LCOMPILERS_API int32_t _lcompilers_blt64(int64_t i, int64_t j) {
    uint64_t ui = i, uj = j;
    return ui < uj;
}

LCOMPILERS_API int32_t _lcompilers_ibits32(int32_t i, int32_t pos, int32_t len) {
    uint32_t ui = i;
    return ((ui << (BITS_32 - pos - len)) >> (BITS_32 - len));
}

LCOMPILERS_API int64_t _lcompilers_ibits64(int64_t i, int32_t pos, int32_t len) {
    uint64_t ui = i;
    return ((ui << (BITS_64 - pos - len)) >> (BITS_64 - len));
}

// cpu_time  -------------------------------------------------------------------

LCOMPILERS_API void _lcompilers_cpu_time(double *t) {
    *t = ((double) clock()) / CLOCKS_PER_SEC;
}

// system_time -----------------------------------------------------------------

LCOMPILERS_API void _lcompilers_i32sys_clock(
        int32_t *count, int32_t *rate, int32_t *max) {
#ifdef _MSC_VER
        *count = - INT_MAX;
        *rate = 0;
        *max = 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        *count = (int32_t)(ts.tv_nsec / 1000000) + ((int32_t)ts.tv_sec * 1000);
        *rate = 1e3; // milliseconds
        *max = INT_MAX;
    } else {
        *count = - INT_MAX;
        *rate = 0;
        *max = 0;
    }
#endif
}

LCOMPILERS_API void _lcompilers_i64sys_clock(
        uint64_t *count, int64_t *rate, int64_t *max) {
#ifdef _MSC_VER
        *count = - INT_MAX;
        *rate = 0;
        *max = 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        *count = (uint64_t)(ts.tv_nsec) + ((uint64_t)ts.tv_sec * 1000000000);
        // FIXME: Rate can be in microseconds or nanoseconds depending on
        //          resolution of the underlying platform clock.
        *rate = 1e9; // nanoseconds
        *max = LLONG_MAX;
    } else {
        *count = - LLONG_MAX;
        *rate = 0;
        *max = 0;
    }
#endif
}

LCOMPILERS_API void _lcompilers_sp_rand_num(float *x) {
    srand(time(0));
    *x = rand() / (float) RAND_MAX;
}

LCOMPILERS_API void _lcompilers_dp_rand_num(double *x) {
    srand(time(0));
    *x = rand() / (double) RAND_MAX;
}

LCOMPILERS_API int64_t _lpython_open(char *path, char *flags)
{
    FILE *fd;
    fd = fopen(path, flags);
    if (!fd)
    {
        printf("Error in opening the file!\n");
        perror(path);
        exit(1);
    }
    return (int64_t)fd;
}

LCOMPILERS_API char* _lpython_read(int64_t fd, int64_t n)
{
    char *c = (char *) calloc(n, sizeof(char));
    if (fd < 0)
    {
        printf("Error in reading the file!\n");
        exit(1);
    }
    int x = fread(c, 1, n, (FILE*)fd);
    c[x] = '\0';
    return c;
}

LCOMPILERS_API void _lpython_close(int64_t fd)
{
    if (fclose((FILE*)fd) != 0)
    {
        printf("Error in closing the file!\n");
        exit(1);
    }
}
