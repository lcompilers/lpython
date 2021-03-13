#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

struct _lfortran_complex {
    float re, im;
};

#if _WIN32
typedef struct _lfortran_complex float_complex_t;
typedef struct _lfortran_complex double_complex_t;
#else
typedef float _Complex float_complex_t;
typedef double _Complex double_complex_t;
#endif


double _lfort_sum(int n, double *v)
{
    int i, r;
    r = 0;
    for (i=0; i < n; i++) {
        r += v[i];
    }
    return r;
}

void _lfort_random_number(int n, double *v)
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

void _lfortran_complex_pow(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result)
{
    #ifdef _WIN32
        result->re = 0.0;
        result->im = 0.0;
    #else
        float complex ca = CMPLXF(a->re, a->im);
        float complex cb = CMPLXF(b->re, b->im);
        float complex cr = cpowf(ca, cb);
        result->re = crealf(cr);
        result->im = cimagf(cr);
    #endif

}

// sin -------------------------------------------------------------------------

void _lfortran_sin(float x, float *result)
{
    *result = sin(x);
}

float _lfortran_ssin(float x)
{
    return sin(x);
}

double _lfortran_dsin(double x)
{
    return sin(x);
}

float_complex_t _lfortran_csin(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return sin(x);
#endif
}

double_complex_t _lfortran_zsin(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return sin(x);
#endif
}

// cos -------------------------------------------------------------------------

void _lfortran_cos(float x, float *result)
{
    *result = cos(x);
}

float _lfortran_scos(float x)
{
    return cos(x);
}

double _lfortran_dcos(double x)
{
    return cos(x);
}

float_complex_t _lfortran_ccos(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return cos(x);
#endif
}

double_complex_t _lfortran_zcos(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return cos(x);
#endif
}

// tan -------------------------------------------------------------------------

void _lfortran_tan(float x, float *result)
{
    *result = tan(x);
}

float _lfortran_stan(float x)
{
    return tan(x);
}

double _lfortran_dtan(double x)
{
    return tan(x);
}

float_complex_t _lfortran_ctan(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return tan(x);
#endif
}

double_complex_t _lfortran_ztan(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return tan(x);
#endif
}

// sinh ------------------------------------------------------------------------

void _lfortran_sinh(float x, float *result)
{
    *result = sinh(x);
}

float _lfortran_ssinh(float x)
{
    return sinh(x);
}

double _lfortran_dsinh(double x)
{
    return sinh(x);
}

float_complex_t _lfortran_csinh(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return sinh(x);
#endif
}

double_complex_t _lfortran_zsinh(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return sinh(x);
#endif
}

// cosh -------------------------------------------------------------------------

void _lfortran_cosh(float x, float *result)
{
    *result = cosh(x);
}

float _lfortran_scosh(float x)
{
    return cosh(x);
}

double _lfortran_dcosh(double x)
{
    return cosh(x);
}

float_complex_t _lfortran_ccosh(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return cosh(x);
#endif
}

double_complex_t _lfortran_zcosh(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return cosh(x);
#endif
}

// tanh -------------------------------------------------------------------------

void _lfortran_tanh(float x, float *result)
{
    *result = tanh(x);
}

float _lfortran_stanh(float x)
{
    return tanh(x);
}

double _lfortran_dtanh(double x)
{
    return tanh(x);
}

float_complex_t _lfortran_ctanh(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return tanh(x);
#endif
}

double_complex_t _lfortran_ztanh(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return tanh(x);
#endif
}
