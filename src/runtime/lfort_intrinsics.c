#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct _lfortran_complex {
    float re, im;
};

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

float _Complex _lfortran_csin(float _Complex x)
{
    return sin(x);
}

double _Complex _lfortran_zsin(double _Complex x)
{
    return sin(x);
}
