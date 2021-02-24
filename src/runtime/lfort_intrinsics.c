#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

struct complex {
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

void _lfortran_complex_add(struct complex* a, struct complex* b,
    struct complex *result)
{
    result->re = a->re + b->re;
    result->im = a->im + b->im;
}
