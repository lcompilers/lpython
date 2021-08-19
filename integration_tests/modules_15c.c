#include "modules_15c.h"

int f_int_float(int *a, float *b) {
    return *a + *b;
}

int f_int_double(int *a, double *b) {
    return *a + *b;
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
