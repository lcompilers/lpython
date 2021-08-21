#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>

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

// exp -------------------------------------------------------------------------

float _lfortran_sexp(float x)
{
    return exp(x);
}

double _lfortran_dexp(double x)
{
    return exp(x);
}

// log -------------------------------------------------------------------------

float _lfortran_slog(float x)
{
    return log(x);
}

double _lfortran_dlog(double x)
{
    return log(x);
}

// erf -------------------------------------------------------------------------

float _lfortran_serf(float x)
{
    return erf(x);
}

double _lfortran_derf(double x)
{
    return erf(x);
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

// asin -------------------------------------------------------------------------

void _lfortran_asin(float x, float *result)
{
    *result = asin(x);
}

float _lfortran_sasin(float x)
{
    return asin(x);
}

double _lfortran_dasin(double x)
{
    return asin(x);
}

float_complex_t _lfortran_casin(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return asin(x);
#endif
}

double_complex_t _lfortran_zasin(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return asin(x);
#endif
}

// acos -------------------------------------------------------------------------

void _lfortran_acos(float x, float *result)
{
    *result = acos(x);
}

float _lfortran_sacos(float x)
{
    return acos(x);
}

double _lfortran_dacos(double x)
{
    return acos(x);
}

float_complex_t _lfortran_cacos(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return acos(x);
#endif
}

double_complex_t _lfortran_zacos(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return acos(x);
#endif
}

// atan -------------------------------------------------------------------------
// TODO: Handle atan(Y,X) and atan2(Y,X)

void _lfortran_atan(float x, float *result)
{
    *result = atan(x);
}

float _lfortran_satan(float x)
{
    return atan(x);
}

double _lfortran_datan(double x)
{
    return atan(x);
}

float_complex_t _lfortran_catan(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return atan(x);
#endif
}

double_complex_t _lfortran_zatan(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return atan(x);
#endif
}

// asinh ------------------------------------------------------------------------

void _lfortran_asinh(float x, float *result)
{
    *result = asinh(x);
}

float _lfortran_sasinh(float x)
{
    return asinh(x);
}

double _lfortran_dasinh(double x)
{
    return asinh(x);
}

float_complex_t _lfortran_casinh(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return asinh(x);
#endif
}

double_complex_t _lfortran_zasinh(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return asinh(x);
#endif
}

// acosh -------------------------------------------------------------------------

void _lfortran_acosh(float x, float *result)
{
    *result = acosh(x);
}

float _lfortran_sacosh(float x)
{
    return acosh(x);
}

double _lfortran_dacosh(double x)
{
    return acosh(x);
}

float_complex_t _lfortran_cacosh(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return acosh(x);
#endif
}

double_complex_t _lfortran_zacosh(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return acosh(x);
#endif
}

// atanh -------------------------------------------------------------------------

void _lfortran_atanh(float x, float *result)
{
    *result = atanh(x);
}

float _lfortran_satanh(float x)
{
    return atanh(x);
}

double _lfortran_datanh(double x)
{
    return atanh(x);
}

float_complex_t _lfortran_catanh(float_complex_t x)
{
#ifdef _WIN32
    float_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return atanh(x);
#endif
}

double_complex_t _lfortran_zatanh(double_complex_t x)
{
#ifdef _WIN32
    double_complex_t r;
    return r; // TODO: implement in MSVC
#else
    return atanh(x);
#endif
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

char* _lfortran_malloc(int size) {
    return (char*)malloc(size);
}

void _lfortran_free(char* ptr) {
    free((void*)ptr);
}
