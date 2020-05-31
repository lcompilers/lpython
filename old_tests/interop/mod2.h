#ifndef MOD2_H
#define MOD2_H

extern "C" {

int32_t __mod1_MOD_f1(int32_t *a, int32_t *b);
int32_t __mod1_MOD_f2(int32_t *n, int32_t *a);
int32_t __mod1_MOD_f2b(descriptor<1, int32_t> *a);
int32_t __mod1_MOD_f3(int32_t *n, int32_t *m, int32_t *a);
int32_t __mod1_MOD_f3b(descriptor<2, int32_t> *a);
int32_t __mod1_MOD_f4(int32_t *a, int32_t *b);
int32_t __mod1_MOD_f5b(descriptor<2, int32_t> *a);
}

#endif // MOD2_H

