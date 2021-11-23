#ifndef LFORTRAN_INTRINSICS_H
#define LFORTRAN_INTRINSICS_H

#include <stdarg.h>
#include <complex.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef _WIN32
#define LFORTRAN_API __declspec(dllexport)
#elif defined(__linux__)
#define LFORTRAN_API __attribute__((visibility("default")))
#else
#define LFORTRAN_API /* Nothing */
#endif

LFORTRAN_API double _lfortran_sum(int n, double *v);
LFORTRAN_API void _lfortran_random_number(int n, double *v);
LFORTRAN_API void _lfortran_printf(const char* format, ...);
LFORTRAN_API void _lfortran_complex_add(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result);
LFORTRAN_API void _lfortran_complex_sub(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result);
LFORTRAN_API void _lfortran_complex_mul(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result);
LFORTRAN_API void _lfortran_complex_div(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result);

LFORTRAN_API void _lfortran_complex_pow(struct _lfortran_complex* a,
        struct _lfortran_complex* b, struct _lfortran_complex *result);
LFORTRAN_API float_complex_t _lfortran_csqrt(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zsqrt(double_complex_t x);
LFORTRAN_API float _lfortran_caimag(float_complex_t x);
LFORTRAN_API double _lfortran_zaimag(double_complex_t x);
LFORTRAN_API float _lfortran_sexp(float x);
LFORTRAN_API double _lfortran_dexp(double x);
LFORTRAN_API float_complex_t _lfortran_cexp(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zexp(double_complex_t x);
LFORTRAN_API float _lfortran_slog(float x);
LFORTRAN_API double _lfortran_dlog(double x);
LFORTRAN_API float_complex_t _lfortran_clog(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zlog(double_complex_t x);
LFORTRAN_API float _lfortran_serf(float x);
LFORTRAN_API double _lfortran_derf(double x);
LFORTRAN_API float _lfortran_serfc(float x);
LFORTRAN_API double _lfortran_derfc(double x);
LFORTRAN_API float _lfortran_slog10(float x);
LFORTRAN_API double _lfortran_dlog10(double x);
LFORTRAN_API float _lfortran_sgamma(float x);
LFORTRAN_API double _lfortran_dgamma(double x);
LFORTRAN_API float _lfortran_slog_gamma(float x);
LFORTRAN_API double _lfortran_dlog_gamma(double x);
LFORTRAN_API float _lfortran_ssin(float x);
LFORTRAN_API double _lfortran_dsin(double x);
LFORTRAN_API float_complex_t _lfortran_csin(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zsin(double_complex_t x);
LFORTRAN_API float _lfortran_scos(float x);
LFORTRAN_API double _lfortran_dcos(double x);
LFORTRAN_API float_complex_t _lfortran_ccos(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zcos(double_complex_t x);
LFORTRAN_API float _lfortran_stan(float x);
LFORTRAN_API double _lfortran_dtan(double x);
LFORTRAN_API float_complex_t _lfortran_ctan(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_ztan(double_complex_t x);
LFORTRAN_API float _lfortran_ssinh(float x);
LFORTRAN_API double _lfortran_dsinh(double x);
LFORTRAN_API float_complex_t _lfortran_csinh(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zsinh(double_complex_t x);
LFORTRAN_API float _lfortran_scosh(float x);
LFORTRAN_API double _lfortran_dcosh(double x);
LFORTRAN_API float_complex_t _lfortran_ccosh(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zcosh(double_complex_t x);
LFORTRAN_API float _lfortran_stanh(float x);
LFORTRAN_API double _lfortran_dtanh(double x);
LFORTRAN_API float_complex_t _lfortran_ctanh(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_ztanh(double_complex_t x);
LFORTRAN_API float _lfortran_sasin(float x);
LFORTRAN_API double _lfortran_dasin(double x);
LFORTRAN_API float_complex_t _lfortran_casin(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zasin(double_complex_t x);
LFORTRAN_API float _lfortran_sacos(float x);
LFORTRAN_API double _lfortran_dacos(double x);
LFORTRAN_API float_complex_t _lfortran_cacos(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zacos(double_complex_t x);
LFORTRAN_API float _lfortran_satan(float x);
LFORTRAN_API double _lfortran_datan(double x);
LFORTRAN_API float_complex_t _lfortran_catan(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zatan(double_complex_t x);
LFORTRAN_API float _lfortran_satan2(float y, float x);
LFORTRAN_API double _lfortran_datan2(double y, double x);
LFORTRAN_API float _lfortran_sasinh(float x);
LFORTRAN_API double _lfortran_dasinh(double x);
LFORTRAN_API float_complex_t _lfortran_casinh(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zasinh(double_complex_t x);
LFORTRAN_API float _lfortran_sacosh(float x);
LFORTRAN_API double _lfortran_dacosh(double x);
LFORTRAN_API float_complex_t _lfortran_cacosh(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zacosh(double_complex_t x);
LFORTRAN_API float _lfortran_satanh(float x);
LFORTRAN_API double _lfortran_datanh(double x);
LFORTRAN_API float_complex_t _lfortran_catanh(float_complex_t x);
LFORTRAN_API double_complex_t _lfortran_zatanh(double_complex_t x);
LFORTRAN_API void _lfortran_strcat(char** s1, char** s2, char** dest);
LFORTRAN_API int _lfortran_str_len(char** s);
LFORTRAN_API char* _lfortran_malloc(int size);
LFORTRAN_API void _lfortran_free(char* ptr);
LFORTRAN_API void _lfortran_string_init(int size_plus_one, char *s);
LFORTRAN_API int32_t _lfortran_iand32(int32_t x, int32_t y);
LFORTRAN_API int64_t _lfortran_iand64(int64_t x, int64_t y);
LFORTRAN_API int32_t _lfortran_ior32(int32_t x, int32_t y);
LFORTRAN_API int64_t _lfortran_ior64(int64_t x, int64_t y);
LFORTRAN_API int32_t _lfortran_ieor32(int32_t x, int32_t y);
LFORTRAN_API int64_t _lfortran_ieor64(int64_t x, int64_t y);
LFORTRAN_API int32_t _lfortran_ibclr32(int32_t i, int pos);
LFORTRAN_API int64_t _lfortran_ibclr64(int64_t i, int pos);
LFORTRAN_API int32_t _lfortran_ibset32(int32_t i, int pos);
LFORTRAN_API int64_t _lfortran_ibset64(int64_t i, int pos);
LFORTRAN_API int32_t _lfortran_btest32(int32_t i, int pos);
LFORTRAN_API int64_t _lfortran_btest64(int64_t i, int pos);
LFORTRAN_API int32_t _lfortran_ishft32(int32_t i, int32_t shift);
LFORTRAN_API int64_t _lfortran_ishft64(int64_t i, int64_t shift);
LFORTRAN_API void _lfortran_cpu_time(double *t);
LFORTRAN_API void _lfortran_i32sys_clock(
        int32_t *count, int32_t *rate, int32_t *max);
LFORTRAN_API void _lfortran_i64sys_clock(
        uint64_t *count, int64_t *rate, int64_t *max);
LFORTRAN_API void _lfortran_sp_rand_num(float *x);
LFORTRAN_API void _lfortran_dp_rand_num(double *x);

#ifdef __cplusplus
}
#endif

#endif
