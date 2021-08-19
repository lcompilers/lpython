#include "modules_15c.h"

int f1(int *a, float *b) {
    return *a + *b;
}

int f2(int *a, double *b) {
    return *a + *b;
}

double f3(int *n, double *b) {
    int i;
    double s;
    s = 0;
    for (i=0; i < *n; i++) {
        s += b[i];
    }
    return s;
}

int f4(int a, double b) {
    return a + b;
}
