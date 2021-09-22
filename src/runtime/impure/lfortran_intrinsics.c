#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <inttypes.h>

struct _lfortran_complex {
    float re, im;
};

#ifdef _MSC_VER
typedef _Fcomplex float_complex_t;
typedef _Dcomplex double_complex_t;
#else
typedef float _Complex float_complex_t;
typedef double _Complex double_complex_t;
#endif


double _lfortran_sum(int n, double *v)
{
    int i, r;
    r = 0;
    for (i=0; i < n; i++) {
        r += v[i];
    }
    return r;
}

void _lfortran_random_number(int n, double *v)
{
    int i;
    for (i=0; i < n; i++) {
        v[i] = rand() / (double) RAND_MAX;
    }
}

void _lfortran_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    fflush(stdout);
    va_end(args);
}

void _lfortran_complex_add(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result)
{
    result->re = a->re + b->re;
    result->im = a->im + b->im;
}

void _lfortran_complex_sub(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result)
{
    result->re = a->re - b->re;
    result->im = a->im - b->im;
}

void _lfortran_complex_mul(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result)
{
    float p = a->re, q = a->im;
    float r = b->re, s = b->im;
    result->re = (p*r - q*s);
    result->im = (p*s + q*r);
}

void _lfortran_complex_div(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result)
{
    float p = a->re, q = a->im;
    float r = b->re, s = -(b->im);
    float mod_b = r*r + s*s;
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

void _lfortran_complex_pow(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result)
{
    #ifdef _MSC_VER
        _Fcomplex ca = _FCOMPLEX_(a->re, a->im);
        _Fcomplex cb = _FCOMPLEX_(b->re, b->im);
        _Fcomplex cr = cpowf(ca, cb);
        result->re = crealf(cr);
        result->im = cimagf(cr);
    #else
        float complex ca = CMPLXF(a->re, a->im);
        float complex cb = CMPLXF(b->re, b->im);
        float complex cr = cpowf(ca, cb);
        result->re = crealf(cr);
        result->im = cimagf(cr);
    #endif

}

// sqrt ------------------------------------------------------------------------

float_complex_t _lfortran_csqrt(float_complex_t x)
{
    return csqrtf(x);
}

double_complex_t _lfortran_zsqrt(double_complex_t x)
{
    return csqrt(x);
}

// exp -------------------------------------------------------------------------

float _lfortran_sexp(float x)
{
    return expf(x);
}

double _lfortran_dexp(double x)
{
    return exp(x);
}

float_complex_t _lfortran_cexp(float_complex_t x)
{
    return cexpf(x);
}

double_complex_t _lfortran_zexp(double_complex_t x)
{
    return cexp(x);
}

// log -------------------------------------------------------------------------

float _lfortran_slog(float x)
{
    return logf(x);
}

double _lfortran_dlog(double x)
{
    return log(x);
}

float_complex_t _lfortran_clog(float_complex_t x)
{
    return clogf(x);
}

double_complex_t _lfortran_zlog(double_complex_t x)
{
    return clog(x);
}

// erf -------------------------------------------------------------------------

float _lfortran_serf(float x)
{
    return erff(x);
}

double _lfortran_derf(double x)
{
    return erf(x);
}

// sin -------------------------------------------------------------------------

float _lfortran_ssin(float x)
{
    return sinf(x);
}

double _lfortran_dsin(double x)
{
    return sin(x);
}

float_complex_t _lfortran_csin(float_complex_t x)
{
    return csinf(x);
}

double_complex_t _lfortran_zsin(double_complex_t x)
{
    return csin(x);
}

// cos -------------------------------------------------------------------------

float _lfortran_scos(float x)
{
    return cosf(x);
}

double _lfortran_dcos(double x)
{
    return cos(x);
}

float_complex_t _lfortran_ccos(float_complex_t x)
{
    return ccosf(x);
}

double_complex_t _lfortran_zcos(double_complex_t x)
{
    return ccos(x);
}

// tan -------------------------------------------------------------------------

float _lfortran_stan(float x)
{
    return tanf(x);
}

double _lfortran_dtan(double x)
{
    return tan(x);
}

float_complex_t _lfortran_ctan(float_complex_t x)
{
    return ctanf(x);
}

double_complex_t _lfortran_ztan(double_complex_t x)
{
    return ctan(x);
}

// sinh ------------------------------------------------------------------------

float _lfortran_ssinh(float x)
{
    return sinhf(x);
}

double _lfortran_dsinh(double x)
{
    return sinh(x);
}

float_complex_t _lfortran_csinh(float_complex_t x)
{
    return csinhf(x);
}

double_complex_t _lfortran_zsinh(double_complex_t x)
{
    return csinh(x);
}

// cosh ------------------------------------------------------------------------


float _lfortran_scosh(float x)
{
    return coshf(x);
}

double _lfortran_dcosh(double x)
{
    return cosh(x);
}

float_complex_t _lfortran_ccosh(float_complex_t x)
{
    return ccoshf(x);
}

double_complex_t _lfortran_zcosh(double_complex_t x)
{
    return ccosh(x);
}

// tanh ------------------------------------------------------------------------

float _lfortran_stanh(float x)
{
    return tanhf(x);
}

double _lfortran_dtanh(double x)
{
    return tanh(x);
}

float_complex_t _lfortran_ctanh(float_complex_t x)
{
    return ctanhf(x);
}

double_complex_t _lfortran_ztanh(double_complex_t x)
{
    return ctanh(x);
}

// asin ------------------------------------------------------------------------

float _lfortran_sasin(float x)
{
    return asinf(x);
}

double _lfortran_dasin(double x)
{
    return asin(x);
}

float_complex_t _lfortran_casin(float_complex_t x)
{
    return casinf(x);
}

double_complex_t _lfortran_zasin(double_complex_t x)
{
    return casin(x);
}

// acos ------------------------------------------------------------------------

float _lfortran_sacos(float x)
{
    return acosf(x);
}

double _lfortran_dacos(double x)
{
    return acos(x);
}

float_complex_t _lfortran_cacos(float_complex_t x)
{
    return cacosf(x);
}

double_complex_t _lfortran_zacos(double_complex_t x)
{
    return cacos(x);
}

// atan ------------------------------------------------------------------------

float _lfortran_satan(float x)
{
    return atanf(x);
}

double _lfortran_datan(double x)
{
    return atan(x);
}

float_complex_t _lfortran_catan(float_complex_t x)
{
    return catanf(x);
}

double_complex_t _lfortran_zatan(double_complex_t x)
{
    return catan(x);
}

// atan2 -----------------------------------------------------------------------

float _lfortran_satan2(float y, float x)
{
    return atan2f(y, x);
}

double _lfortran_datan2(double y, double x)
{
    return atan2(y, x);
}

// asinh -----------------------------------------------------------------------

float _lfortran_sasinh(float x)
{
    return asinhf(x);
}

double _lfortran_dasinh(double x)
{
    return asinh(x);
}

float_complex_t _lfortran_casinh(float_complex_t x)
{
    return casinhf(x);
}

double_complex_t _lfortran_zasinh(double_complex_t x)
{
    return casinh(x);
}

// acosh -----------------------------------------------------------------------

float _lfortran_sacosh(float x)
{
    return acoshf(x);
}

double _lfortran_dacosh(double x)
{
    return acosh(x);
}

float_complex_t _lfortran_cacosh(float_complex_t x)
{
    return cacoshf(x);
}

double_complex_t _lfortran_zacosh(double_complex_t x)
{
    return cacosh(x);
}

// atanh -----------------------------------------------------------------------

float _lfortran_satanh(float x)
{
    return atanhf(x);
}

double _lfortran_datanh(double x)
{
    return atanh(x);
}

float_complex_t _lfortran_catanh(float_complex_t x)
{
    return catanhf(x);
}

double_complex_t _lfortran_zatanh(double_complex_t x)
{
    return catanh(x);
}


// strcat  --------------------------------------------------------------------

void _lfortran_strcat(char** s1, char** s2, char** dest)
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

int _lfortran_str_len(char** s)
{
    return strlen(*s);
}

char* _lfortran_malloc(int size) {
    return (char*)malloc(size);
}

void _lfortran_free(char* ptr) {
    free((void*)ptr);
}

// size_plus_one is the size of the string including the null character
void _lfortran_string_init(int size_plus_one, char *s) {
    int size = size_plus_one-1;
    for (int i=0; i < size; i++) {
        s[i] = ' ';
    }
    s[size] = '\0';
}

int32_t _lfortran_iand32(int32_t x, int32_t y) {
    return x & y;
}

int64_t _lfortran_iand64(int64_t x, int64_t y) {
    return x & y;
}
