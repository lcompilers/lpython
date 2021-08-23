#include <complex.h>

#include "modules_15c.h"

int f_int_float(int *a, float *b) {
    return *a + *b;
}

int f_int_double(int *a, double *b) {
    return *a + *b;
}

int f_int_float_complex(int *a, float_complex_t *b) {
    return *a + crealf(*b) + cimagf(*b);
}

int f_int_double_complex(int *a, double_complex_t *b) {
    return *a + creal(*b) + cimag(*b);
}

int f_int_float_complex_value(int a, float_complex_t b) {
    return a + crealf(b) + cimagf(b);
}

int f_int_double_complex_value(int a, double_complex_t b) {
    return a + creal(b) + cimag(b);
}

float_complex_t f_float_complex_value_return(float_complex_t b) {
    float_complex_t r;
    r = b * 2;
    return r;
}

double_complex_t f_double_complex_value_return(double_complex_t b) {
    double_complex_t r;
    r = b * 2;
    return r;
}

int f_int_float_value(int a, float b) {
    return a + b;
}

int f_int_double_value(int a, double b) {
    return a + b;
}

int f_int_intarray(int n, int *b) {
    int i;
    int s;
    s = 0;
    for (i=0; i < n; i++) {
        s += b[i];
    }
    return s;
}

float f_int_floatarray(int n, float *b) {
    int i;
    float s;
    s = 0;
    for (i=0; i < n; i++) {
        s += b[i];
    }
    return s;
}

double f_int_doublearray(int n, double *b) {
    int i;
    double s;
    s = 0;
    for (i=0; i < n; i++) {
        s += b[i];
    }
    return s;
}

// --------------------------------------------------------------------

void sub_int_float(int *a, float *b, int *r) {
    *r = *a + *b;
}

void sub_int_double(int *a, double *b, int *r) {
    *r = *a + *b;
}

void sub_int_float_complex(int *a, float_complex_t *b, int *r) {
    *r = *a + crealf(*b) + cimagf(*b);
}

void sub_int_double_complex(int *a, double_complex_t *b, int *r) {
    *r = *a + creal(*b) + cimag(*b);
}

void sub_int_float_value(int a, float b, int *r) {
    *r = a + b;
}

void sub_int_double_value(int a, double b, int *r) {
    *r = a + b;
}

void sub_int_float_complex_value(int a, float_complex_t b, int *r) {
    *r = a + crealf(b) + cimagf(b);
}

void sub_int_double_complex_value(int a, double_complex_t b, int *r) {
    *r = a + creal(b) + cimag(b);
}

void sub_int_intarray(int n, int *b, int *r) {
    int i;
    int s;
    s = 0;
    for (i=0; i < n; i++) {
        s += b[i];
    }
    *r = s;
}

void sub_int_floatarray(int n, float *b, float *r) {
    int i;
    float s;
    s = 0;
    for (i=0; i < n; i++) {
        s += b[i];
    }
    *r = s;
}

void sub_int_doublearray(int n, double *b, double *r) {
    int i;
    double s;
    s = 0;
    for (i=0; i < n; i++) {
        s += b[i];
    }
    *r = s;
}
