#ifndef LCOMPILERS_INTRINSICS_H
#define LCOMPILERS_INTRINSICS_H

#include <stdarg.h>
#include <complex.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _lcompilers_complex_32 {
    float re, im;
};

struct _lcompilers_complex_64 {
    double re, im;
};

#ifdef _MSC_VER
typedef _Fcomplex float_complex_t;
typedef _Dcomplex double_complex_t;
#else
typedef float _Complex float_complex_t;
typedef double _Complex double_complex_t;
#endif

#ifdef _WIN32
#define LCOMPILERS_API __declspec(dllexport)
#elif defined(__linux__)
#define LCOMPILERS_API __attribute__((visibility("default")))
#else
#define LCOMPILERS_API /* Nothing */
#endif

LCOMPILERS_API double _lcompilers_sum(int n, double *v);
LCOMPILERS_API void _lcompilers_random_number(int n, double *v);
LCOMPILERS_API double _lcompilers_random();
LCOMPILERS_API int _lcompilers_randrange(int lower, int upper);
LCOMPILERS_API int _lcompilers_random_int(int lower, int upper);
LCOMPILERS_API void _lcompilers_printf(const char* format, ...);

LCOMPILERS_API void _lcompilers_complex_add_32(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32 *result);
LCOMPILERS_API void _lcompilers_complex_sub(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32 *result);
LCOMPILERS_API void _lcompilers_complex_mul(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32 *result);
LCOMPILERS_API void _lcompilers_complex_div(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32* result);
LCOMPILERS_API void _lcompilers_complex_pow(struct _lcompilers_complex_32* a,
        struct _lcompilers_complex_32* b, struct _lcompilers_complex_32* result);

LCOMPILERS_API void _lcompilers_complex_add_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result);
LCOMPILERS_API void _lcompilers_complex_sub_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result);
LCOMPILERS_API void _lcompilers_complex_mul_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result);
LCOMPILERS_API void _lcompilers_complex_div_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result);
LCOMPILERS_API void _lcompilers_complex_pow_64(struct _lcompilers_complex_64* a,
        struct _lcompilers_complex_64* b, struct _lcompilers_complex_64 *result);
LCOMPILERS_API void _lcompilers_complex_aimag_32(struct _lcompilers_complex_32 *x, float *res);
LCOMPILERS_API void _lcompilers_complex_aimag_64(struct _lcompilers_complex_64 *x, double *res);
LCOMPILERS_API float_complex_t _lcompilers_csqrt(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zsqrt(double_complex_t x);
LCOMPILERS_API float _lcompilers_caimag(float_complex_t x);
LCOMPILERS_API double _lcompilers_zaimag(double_complex_t x);
LCOMPILERS_API float _lcompilers_sexp(float x);
LCOMPILERS_API double _lcompilers_dexp(double x);
LCOMPILERS_API float_complex_t _lcompilers_cexp(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zexp(double_complex_t x);
LCOMPILERS_API float _lcompilers_slog(float x);
LCOMPILERS_API double _lcompilers_dlog(double x);
LCOMPILERS_API float_complex_t _lcompilers_clog(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zlog(double_complex_t x);
LCOMPILERS_API float _lcompilers_serf(float x);
LCOMPILERS_API double _lcompilers_derf(double x);
LCOMPILERS_API float _lcompilers_serfc(float x);
LCOMPILERS_API double _lcompilers_derfc(double x);
LCOMPILERS_API float _lcompilers_slog10(float x);
LCOMPILERS_API double _lcompilers_dlog10(double x);
LCOMPILERS_API float _lcompilers_sgamma(float x);
LCOMPILERS_API double _lcompilers_dgamma(double x);
LCOMPILERS_API float _lcompilers_slog_gamma(float x);
LCOMPILERS_API double _lcompilers_dlog_gamma(double x);
LCOMPILERS_API float _lcompilers_ssin(float x);
LCOMPILERS_API double _lcompilers_dsin(double x);
LCOMPILERS_API float_complex_t _lcompilers_csin(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zsin(double_complex_t x);
LCOMPILERS_API float _lcompilers_scos(float x);
LCOMPILERS_API double _lcompilers_dcos(double x);
LCOMPILERS_API float_complex_t _lcompilers_ccos(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zcos(double_complex_t x);
LCOMPILERS_API float _lcompilers_stan(float x);
LCOMPILERS_API double _lcompilers_dtan(double x);
LCOMPILERS_API float_complex_t _lcompilers_ctan(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_ztan(double_complex_t x);
LCOMPILERS_API float _lcompilers_ssinh(float x);
LCOMPILERS_API double _lcompilers_dsinh(double x);
LCOMPILERS_API float_complex_t _lcompilers_csinh(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zsinh(double_complex_t x);
LCOMPILERS_API float _lcompilers_scosh(float x);
LCOMPILERS_API double _lcompilers_dcosh(double x);
LCOMPILERS_API float_complex_t _lcompilers_ccosh(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zcosh(double_complex_t x);
LCOMPILERS_API float _lcompilers_stanh(float x);
LCOMPILERS_API double _lcompilers_dtanh(double x);
LCOMPILERS_API float_complex_t _lcompilers_ctanh(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_ztanh(double_complex_t x);
LCOMPILERS_API float _lcompilers_sasin(float x);
LCOMPILERS_API double _lcompilers_dasin(double x);
LCOMPILERS_API float_complex_t _lcompilers_casin(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zasin(double_complex_t x);
LCOMPILERS_API float _lcompilers_sacos(float x);
LCOMPILERS_API double _lcompilers_dacos(double x);
LCOMPILERS_API float_complex_t _lcompilers_cacos(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zacos(double_complex_t x);
LCOMPILERS_API float _lcompilers_satan(float x);
LCOMPILERS_API double _lcompilers_datan(double x);
LCOMPILERS_API float_complex_t _lcompilers_catan(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zatan(double_complex_t x);
LCOMPILERS_API float _lcompilers_satan2(float y, float x);
LCOMPILERS_API double _lcompilers_datan2(double y, double x);
LCOMPILERS_API float _lcompilers_sasinh(float x);
LCOMPILERS_API double _lcompilers_dasinh(double x);
LCOMPILERS_API float_complex_t _lcompilers_casinh(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zasinh(double_complex_t x);
LCOMPILERS_API float _lcompilers_sacosh(float x);
LCOMPILERS_API double _lcompilers_dacosh(double x);
LCOMPILERS_API float_complex_t _lcompilers_cacosh(float_complex_t x);
LCOMPILERS_API double _lcompilers_dfmod(double x, double y);
LCOMPILERS_API double_complex_t _lcompilers_zacosh(double_complex_t x);
LCOMPILERS_API float _lcompilers_satanh(float x);
LCOMPILERS_API double _lcompilers_datanh(double x);
LCOMPILERS_API float_complex_t _lcompilers_catanh(float_complex_t x);
LCOMPILERS_API double_complex_t _lcompilers_zatanh(double_complex_t x);
LCOMPILERS_API void _lcompilers_strrepeat(char** s, int32_t n, char** dest);
LCOMPILERS_API void _lcompilers_strcat(char** s1, char** s2, char** dest);
LCOMPILERS_API int _lcompilers_str_len(char** s);
LCOMPILERS_API char* _lcompilers_malloc(int size);
LCOMPILERS_API void _lcompilers_free(char* ptr);
LCOMPILERS_API void _lcompilers_string_init(int size_plus_one, char *s);
LCOMPILERS_API int32_t _lcompilers_iand32(int32_t x, int32_t y);
LCOMPILERS_API int64_t _lcompilers_iand64(int64_t x, int64_t y);
LCOMPILERS_API int32_t _lcompilers_not32(int32_t x);
LCOMPILERS_API int64_t _lcompilers_not64(int64_t x);
LCOMPILERS_API int32_t _lcompilers_ior32(int32_t x, int32_t y);
LCOMPILERS_API int64_t _lcompilers_ior64(int64_t x, int64_t y);
LCOMPILERS_API int32_t _lcompilers_ieor32(int32_t x, int32_t y);
LCOMPILERS_API int64_t _lcompilers_ieor64(int64_t x, int64_t y);
LCOMPILERS_API int32_t _lcompilers_ibclr32(int32_t i, int pos);
LCOMPILERS_API int64_t _lcompilers_ibclr64(int64_t i, int pos);
LCOMPILERS_API int32_t _lcompilers_ibset32(int32_t i, int pos);
LCOMPILERS_API int64_t _lcompilers_ibset64(int64_t i, int pos);
LCOMPILERS_API int32_t _lcompilers_btest32(int32_t i, int pos);
LCOMPILERS_API int64_t _lcompilers_btest64(int64_t i, int pos);
LCOMPILERS_API int32_t _lcompilers_ishft32(int32_t i, int32_t shift);
LCOMPILERS_API int64_t _lcompilers_ishft64(int64_t i, int64_t shift);
LCOMPILERS_API int32_t _lcompilers_mvbits32(int32_t from, int32_t frompos,
                                        int32_t len, int32_t to, int32_t topos);
LCOMPILERS_API int64_t _lcompilers_mvbits64(int64_t from, int32_t frompos,
                                        int32_t len, int64_t to, int32_t topos);
LCOMPILERS_API int32_t _lcompilers_bgt32(int32_t i, int32_t j);
LCOMPILERS_API int32_t _lcompilers_bgt64(int64_t i, int64_t j);
LCOMPILERS_API int32_t _lcompilers_bge32(int32_t i, int32_t j);
LCOMPILERS_API int32_t _lcompilers_bge64(int64_t i, int64_t j);
LCOMPILERS_API int32_t _lcompilers_ble32(int32_t i, int32_t j);
LCOMPILERS_API int32_t _lcompilers_ble64(int64_t i, int64_t j);
LCOMPILERS_API int32_t _lcompilers_blt32(int32_t i, int32_t j);
LCOMPILERS_API int32_t _lcompilers_blt64(int64_t i, int64_t j);
LCOMPILERS_API int32_t _lcompilers_ibits32(int32_t i, int32_t pos, int32_t len);
LCOMPILERS_API int64_t _lcompilers_ibits64(int64_t i, int32_t pos, int32_t len);
LCOMPILERS_API void _lcompilers_cpu_time(double *t);
LCOMPILERS_API void _lcompilers_i32sys_clock(
        int32_t *count, int32_t *rate, int32_t *max);
LCOMPILERS_API void _lcompilers_i64sys_clock(
        uint64_t *count, int64_t *rate, int64_t *max);
LCOMPILERS_API void _lcompilers_sp_rand_num(float *x);
LCOMPILERS_API void _lcompilers_dp_rand_num(double *x);
LCOMPILERS_API int64_t _lpython_open(char *path, char *flags);
LCOMPILERS_API char* _lpython_read(int64_t fd, int64_t n);
LCOMPILERS_API void _lpython_close(int64_t fd);

#ifdef __cplusplus
}
#endif

#endif
