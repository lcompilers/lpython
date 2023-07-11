#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>

#if defined(_MSC_VER)
#  include <winsock2.h>
#endif

#include <libasr/runtime/lfortran_intrinsics.h>
#include <libasr/config.h>

#ifdef HAVE_RUNTIME_STACKTRACE

#ifdef HAVE_LFORTRAN_LINK
// For dl_iterate_phdr() functionality
#  include <link.h>
struct dl_phdr_info {
    ElfW(Addr) dlpi_addr;
    const char *dlpi_name;
    const ElfW(Phdr) *dlpi_phdr;
    ElfW(Half) dlpi_phnum;
};
extern int dl_iterate_phdr (int (*__callback) (struct dl_phdr_info *,
    size_t, void *), void *__data);
#endif

#ifdef HAVE_LFORTRAN_UNWIND
// For _Unwind_Backtrace() function
#  include <unwind.h>
#endif

#ifdef HAVE_LFORTRAN_MACHO
#  include <mach-o/dyld.h>
#endif

// Runtime Stacktrace
#define LCOMPILERS_MAX_STACKTRACE_LENGTH 200
char *source_filename;
char *binary_executable_path = "/proc/self/exe";

struct Stacktrace {
    uintptr_t pc[LCOMPILERS_MAX_STACKTRACE_LENGTH];
    uint64_t pc_size;
    uintptr_t current_pc;

    uintptr_t local_pc[LCOMPILERS_MAX_STACKTRACE_LENGTH];
    char *binary_filename[LCOMPILERS_MAX_STACKTRACE_LENGTH];
    uint64_t local_pc_size;

    uint64_t addresses[LCOMPILERS_MAX_STACKTRACE_LENGTH];
    uint64_t line_numbers[LCOMPILERS_MAX_STACKTRACE_LENGTH];
    uint64_t stack_size;
};

// Styles and Colors
#define DIM "\033[2m"
#define BOLD "\033[1m"
#define S_RESET "\033[0m"
#define MAGENTA "\033[35m"
#define C_RESET "\033[39m"

#endif // HAVE_RUNTIME_STACKTRACE

LFORTRAN_API double _lfortran_sum(int n, double *v)
{
    int i, r;
    r = 0;
    for (i=0; i < n; i++) {
        r += v[i];
    }
    return r;
}

LFORTRAN_API void _lfortran_random_number(int n, double *v)
{
    int i;
    for (i=0; i < n; i++) {
        v[i] = rand() / (double) RAND_MAX;
    }
}

LFORTRAN_API double _lfortran_random()
{
    return (rand() / (double) RAND_MAX);
}

LFORTRAN_API int _lfortran_randrange(int lower, int upper)
{
    int rr = lower + (rand() % (upper - lower));
    return rr;
}

LFORTRAN_API int _lfortran_random_int(int lower, int upper)
{
    int randint = lower + (rand() % (upper - lower + 1));
    return randint;
}

LFORTRAN_API void _lfortran_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    fflush(stdout);
    va_end(args);
}

char* substring(const char* str, int start, int end) {
    int len = end - start;
    char* substr = (char*)malloc((len + 1) * sizeof(char));
    strncpy(substr, str + start, len);
    substr[len] = '\0';
    return substr;
}

char* append_to_string(char* str, const char* append) {
    int len1 = strlen(str);
    int len2 = strlen(append);
    str = (char*)realloc(str, (len1 + len2 + 1) * sizeof(char));
    strcat(str, append);
    return str;
}

void handle_integer(char* format, int val, char** result) {
    int width = 0, min_width = 0;
    char* dot_pos = strchr(format, '.');
    if (dot_pos != NULL) {
        dot_pos++;
        width = atoi(format + 1);
        min_width = atoi(dot_pos);
        if (min_width > width) {
            perror("Minimum number of digits cannot be more than the specified width for format.\n");
        }
    } else {
        width = atoi(format + 1);
    }

    int len = (val == 0) ? 1 : (int)log10(abs(val)) + 1;
    if (width >= len) {
        if (min_width > len) {
            for (int i = 0; i < (width - min_width); i++) {
                *result = append_to_string(*result, " ");
            }
            for (int i = 0; i < (min_width - len); i++) {
                *result = append_to_string(*result, "0");
            }
        } else {
            for (int i = 0; i < (width - len); i++) {
                *result = append_to_string(*result, " ");
            }
        }
        char str[20];
        sprintf(str, "%d", val);
        *result = append_to_string(*result, str);
    } else if (width < len) {
        for (int i = 0; i < width; i++) {
            *result = append_to_string(*result, "*");
        }
    }
}

void handle_decimal(char* format, double val, int scale, char** result, char* c) {
    int width = 0, decimal_digits = 0;
    int64_t integer_part = (int64_t)val;
    int integer_length = (integer_part == 0) ? 0 : (int)log10(llabs(integer_part)) + 1;

    char val_str[64];
    sprintf(val_str, "%lf", val);

    int i = strlen(val_str) - 1;
    while (val_str[i] == '0') {
        val_str[i] = '\0';
        i--;
    }

    char* ptr = strchr(val_str, '.');
    if (ptr != NULL) {
        memmove(ptr, ptr + 1, strlen(ptr));
    }

    if (val < 0) {
        memmove(val_str, val_str + 1, strlen(val_str));
    }

    int decimal = -1;
    while (val_str[0] == '0') {
        memmove(val_str, val_str + 1, strlen(val_str));
        decimal++;
    }

    char* dot_pos = strchr(format, '.');
    if (dot_pos != NULL) {
        dot_pos++;
        width = atoi(format + 1);
        decimal_digits = atoi(dot_pos);
        if (decimal_digits > width - 3) {
            perror("Specified width is not enough for the specified number of decimal digits\n");
        }
    } else {
        width = atoi(format + 1);
    }
    if (decimal_digits > strlen(val_str)) {
        for(int i=0; i < decimal_digits - integer_length; i++) {
            strcat(val_str, "0");
        }
    }

    char formatted_value[64] = "";
    int sign_width = (val < 0) ? 1 : 0;
    int spaces = width - sign_width - decimal_digits - 6;
    if (scale > 1){
        decimal_digits -= scale - 1;
    }
    for (int i = 0; i < spaces; i++) {
        strcat(formatted_value, " ");
    }

    if (sign_width == 1) {
        strcat(formatted_value, "-");
    }
    if (scale <= 0) {
        strcat(formatted_value, "0.");
        for (int k = 0; k < abs(scale); k++) {
            strcat(formatted_value, "0");
        }
        if (decimal_digits + scale < strlen(val_str)) {
            int t = round((float)atoi(val_str) / pow(10, (strlen(val_str) - decimal_digits - scale)));
            sprintf(val_str, "%d", t);
        }
        strncat(formatted_value, val_str, decimal_digits + scale);
    } else {
        strcat(formatted_value, substring(val_str, 0, scale));
        strcat(formatted_value, ".");
        char* new_str = substring(val_str, scale, strlen(val_str));
        if (decimal_digits < strlen(new_str)) {
            int t = round((float)atoi(new_str) / pow(10, (strlen(new_str) - decimal_digits)));
            sprintf(new_str, "%d", t);
        }
        strcat(formatted_value, substring(new_str, 0, decimal_digits));
    }

    strcat(formatted_value, c);

    char exponent[12];
    sprintf(exponent, "%+03d", (integer_length > 0 ? integer_length : decimal) - scale);

    strcat(formatted_value, exponent);

    if (strlen(formatted_value) == width + 1 && scale <= 0) {
        char* ptr = strchr(formatted_value, '0');
        if (ptr != NULL) {
            memmove(ptr, ptr + 1, strlen(ptr));
        }
    }

    if (strlen(formatted_value) > width) {
        for(int i=0; i<width; i++){
            *result = append_to_string(*result,"*");
        }
    } else {
        *result = append_to_string(*result, formatted_value);
    }
}

LFORTRAN_API char* _lcompilers_string_format_fortran(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char* modified_input_string = substring(format, 1, strlen(format) - 1);
    char** format_values = NULL;
    int format_values_count = 0;
    char* token = strtok(modified_input_string, ",");
    while (token != NULL) {
        format_values = (char**)realloc(format_values, (format_values_count + 1) * sizeof(char*));
        format_values[format_values_count++] = token;
        token = strtok(NULL, ",");
    }
    char* result = (char*)malloc(sizeof(char));
    result[0] = '\0';
    int arguments = 0;
    for (int i = 0; i < format_values_count; i++) {
        char* value = format_values[i];

        if (value[0] == '/') {
            // Slash Editing (newlines)
            int j = 0;
            while (value[j] == '/') {
                result = append_to_string(result, "\n");
                j++;
            }
            value = substring(value, j, strlen(value));
        }

        int newline = 0;
        if (value[strlen(value) - 1] == '/') {
            // Newlines at the end of the argument
            int j = strlen(value) - 1;
            while (value[j] == '/') {
                newline++;
                j--;
            }
            value = substring(value, 0, strlen(value) - newline);
        }

        int scale = 0;
        if (isdigit(value[0]) && tolower(value[1]) == 'p') {
            // Scale Factor (nP)
            scale = atoi(&value[0]);
            value = substring(value, 2, strlen(value));
        } else if (value[0] == '-' && isdigit(value[1]) && tolower(value[2]) == 'p') {
            scale = atoi(substring(value, 0, 2));
            value = substring(value, 3, strlen(value));
        }

        if (isdigit(value[0])) {
            // Repeat Count
            int j = 0;
            while (isdigit(value[j])) {
                j++;
            }
            int repeat = atoi(substring(value, 0, j));
            if (value[j] == '(') {
                value = substring(value, 1, strlen(value));
                format_values[i] = substring(format_values[i], 1, strlen(format_values[i]));
                char* new_input_string = (char*)malloc(sizeof(char));
                new_input_string[0] = '\0';
                for (int k = i; k < format_values_count; k++) {
                    new_input_string = append_to_string(new_input_string, format_values[k]);
                    new_input_string = append_to_string(new_input_string, ",");
                }
                new_input_string = substring(new_input_string, 1, strchr(new_input_string, ')') - new_input_string);
                char** new_fmt_val = NULL;
                int new_fmt_val_count = 0;
                char* new_token = strtok(new_input_string, ",");
                while (new_token != NULL) {
                    new_fmt_val = (char**)realloc(new_fmt_val, (new_fmt_val_count + 1) * sizeof(char*));
                    new_fmt_val[new_fmt_val_count++] = new_token;
                    new_token = strtok(NULL, ",");
                }
                for (int p = 0; p < repeat - 1; p++) {
                    for (int k = 0; k < new_fmt_val_count; k++) {
                        int f = i + new_fmt_val_count + k;
                        format_values = (char**)realloc(format_values, (format_values_count + 1) * sizeof(char*));
                        memmove(format_values + f + 1, format_values + f, (format_values_count - f) * sizeof(char*));
                        format_values[f] = new_fmt_val[k];
                        format_values_count++;
                    }
                }
            } else if (tolower(value[j]) != 'x') {
                value = substring(value, j, strlen(value));
                for (int k = 0; k < repeat - 1; k++) {
                    format_values = (char**)realloc(format_values, (format_values_count + 1) * sizeof(char*));
                    memmove(format_values + i + 2, format_values + i + 1, (format_values_count - i - 1) * sizeof(char*));
                    format_values[i + 1] = value;
                    format_values_count++;
                }
            }
        }
        if (value[0] == '(') {
            value = substring(value, 1, strlen(value));
        } else if (value[strlen(value)-1] == ')') {
            value = substring(value, 0, strlen(value) - 1);
        }

        if (value[0] == '\"' && value[strlen(value) - 1] == '\"') {
            // String
            value = substring(value, 1, strlen(value) - 1);
            result = append_to_string(result, value);
        } else if (tolower(value[0]) == 'a') {
            // Character Editing (A[n])
            char* str = substring(value, 1, strlen(value));
            char* arg = va_arg(args, char*);
            if (strlen(str) == 0) {
                sprintf(str, "%lu", strlen(arg));
            }
            char* s = (char*)malloc((strlen(str) + 4) * sizeof(char));
            sprintf(s, "%%%s.%ss", str, str);
            char* string = (char*)malloc((strlen(arg)) * sizeof(char));
            sprintf(string, s, arg);
            result = append_to_string(result, string);
            free(s);
            free(string);
        } else if (tolower(value[strlen(value) - 1]) == 'x') {
            // Positional Editing (nX)
            int t = atoi(substring(value, 0, strlen(value) - 1));
            for (int i = 0; i < t; i++) {
                result = append_to_string(result, " ");
            }
        } else if (tolower(value[0]) == 'i') {
            // Integer Editing ( I[w[.m]] )
            int val = va_arg(args, int);
            handle_integer(value, val, &result);
            arguments++;
        } else if (tolower(value[0]) == 'd') {
            // D Editing (D[w[.d]])
            double val = va_arg(args, double);
            handle_decimal(value, val, scale, &result, "D");
            arguments++;
        } else if (tolower(value[0]) == 'e') {
            // E Editing E[w[.d][Ee]]
            // Only (E[w[.d]]) has been implemented yet
            double val = va_arg(args, double);
            handle_decimal(value, val, scale, &result, "E");
            arguments++;
        } else if (strlen(value) != 0) {
            printf("Printing support is not available for %s format.\n",value);
        }

        while (newline != 0) {
            result = append_to_string(result, " ");
            newline--;
        }
    }

    free(modified_input_string);
    free(format_values);
    va_end(args);
    return result;
}

LFORTRAN_API void _lcompilers_print_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fflush(stderr);
    va_end(args);
}

LFORTRAN_API void _lfortran_complex_add_32(struct _lfortran_complex_32* a,
        struct _lfortran_complex_32* b, struct _lfortran_complex_32 *result)
{
    result->re = a->re + b->re;
    result->im = a->im + b->im;
}

LFORTRAN_API void _lfortran_complex_add_64(struct _lfortran_complex_64* a,
        struct _lfortran_complex_64* b, struct _lfortran_complex_64 *result)
{
    result->re = a->re + b->re;
    result->im = a->im + b->im;
}

LFORTRAN_API void _lfortran_complex_sub_32(struct _lfortran_complex_32* a,
        struct _lfortran_complex_32* b, struct _lfortran_complex_32 *result)
{
    result->re = a->re - b->re;
    result->im = a->im - b->im;
}

LFORTRAN_API void _lfortran_complex_sub_64(struct _lfortran_complex_64* a,
        struct _lfortran_complex_64* b, struct _lfortran_complex_64 *result)
{
    result->re = a->re - b->re;
    result->im = a->im - b->im;
}

LFORTRAN_API void _lfortran_complex_mul_32(struct _lfortran_complex_32* a,
        struct _lfortran_complex_32* b, struct _lfortran_complex_32 *result)
{
    float p = a->re, q = a->im;
    float r = b->re, s = b->im;
    result->re = (p*r - q*s);
    result->im = (p*s + q*r);
}

LFORTRAN_API void _lfortran_complex_mul_64(struct _lfortran_complex_64* a,
        struct _lfortran_complex_64* b, struct _lfortran_complex_64 *result)
{
    double p = a->re, q = a->im;
    double r = b->re, s = b->im;
    result->re = (p*r - q*s);
    result->im = (p*s + q*r);
}

LFORTRAN_API void _lfortran_complex_div_32(struct _lfortran_complex_32* a,
        struct _lfortran_complex_32* b, struct _lfortran_complex_32 *result)
{
    float p = a->re, q = a->im;
    float r = b->re, s = -(b->im);
    float mod_b = r*r + s*s;
    result->re = (p*r - q*s)/mod_b;
    result->im = (p*s + q*r)/mod_b;
}

LFORTRAN_API void _lfortran_complex_div_64(struct _lfortran_complex_64* a,
        struct _lfortran_complex_64* b, struct _lfortran_complex_64 *result)
{
    double p = a->re, q = a->im;
    double r = b->re, s = -(b->im);
    double mod_b = r*r + s*s;
    result->re = (p*r - q*s)/mod_b;
    result->im = (p*s + q*r)/mod_b;
}

#undef CMPLX
#undef CMPLXF
#undef CMPLXL
#undef _Imaginary_I

#define _Imaginary_I (I)
#define CMPLX(x, y) ((double complex)((double)(x) + _Imaginary_I * (double)(y)))
#define CMPLXF(x, y) ((float complex)((float)(x) + _Imaginary_I * (float)(y)))
#define CMPLXL(x, y) ((long double complex)((long double)(x) + \
                      _Imaginary_I * (long double)(y)))
#define BITS_32 32
#define BITS_64 64

LFORTRAN_API void _lfortran_complex_pow_32(struct _lfortran_complex_32* a,
        struct _lfortran_complex_32* b, struct _lfortran_complex_32 *result)
{
    #ifdef _MSC_VER
        _Fcomplex ca = _FCOMPLEX_(a->re, a->im);
        _Fcomplex cb = _FCOMPLEX_(b->re, b->im);
        _Fcomplex cr = cpowf(ca, cb);
    #else
        float complex ca = CMPLXF(a->re, a->im);
        float complex cb = CMPLXF(b->re, b->im);
        float complex cr = cpowf(ca, cb);
    #endif
        result->re = crealf(cr);
        result->im = cimagf(cr);

}

LFORTRAN_API void _lfortran_complex_pow_64(struct _lfortran_complex_64* a,
        struct _lfortran_complex_64* b, struct _lfortran_complex_64 *result)
{
    #ifdef _MSC_VER
        _Dcomplex ca = _DCOMPLEX_(a->re, a->im);
        _Dcomplex cb = _DCOMPLEX_(b->re, b->im);
        _Dcomplex cr = cpow(ca, cb);
    #else
        double complex ca = CMPLX(a->re, a->im);
        double complex cb = CMPLX(b->re, b->im);
        double complex cr = cpow(ca, cb);
    #endif
        result->re = creal(cr);
        result->im = cimag(cr);

}

// sqrt ------------------------------------------------------------------------

LFORTRAN_API float_complex_t _lfortran_csqrt(float_complex_t x)
{
    return csqrtf(x);
}

LFORTRAN_API double_complex_t _lfortran_zsqrt(double_complex_t x)
{
    return csqrt(x);
}

// aimag -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_caimag(float_complex_t x)
{
    return cimagf(x);
}

LFORTRAN_API double _lfortran_zaimag(double_complex_t x)
{
    return cimag(x);
}

LFORTRAN_API void _lfortran_complex_aimag_32(struct _lfortran_complex_32 *x, float *res)
{
    *res = x->im;
}

LFORTRAN_API void _lfortran_complex_aimag_64(struct _lfortran_complex_64 *x, double *res)
{
    *res = x->im;
}

// exp -------------------------------------------------------------------------

LFORTRAN_API float _lfortran_sexp(float x)
{
    return expf(x);
}

LFORTRAN_API double _lfortran_dexp(double x)
{
    return exp(x);
}

LFORTRAN_API float_complex_t _lfortran_cexp(float_complex_t x)
{
    return cexpf(x);
}

LFORTRAN_API double_complex_t _lfortran_zexp(double_complex_t x)
{
    return cexp(x);
}

// log -------------------------------------------------------------------------

LFORTRAN_API float _lfortran_slog(float x)
{
    return logf(x);
}

LFORTRAN_API double _lfortran_dlog(double x)
{
    return log(x);
}

LFORTRAN_API float_complex_t _lfortran_clog(float_complex_t x)
{
    return clogf(x);
}

LFORTRAN_API double_complex_t _lfortran_zlog(double_complex_t x)
{
    return clog(x);
}

// erf -------------------------------------------------------------------------

LFORTRAN_API float _lfortran_serf(float x)
{
    return erff(x);
}

LFORTRAN_API double _lfortran_derf(double x)
{
    return erf(x);
}

// erfc ------------------------------------------------------------------------

LFORTRAN_API float _lfortran_serfc(float x)
{
    return erfcf(x);
}

LFORTRAN_API double _lfortran_derfc(double x)
{
    return erfc(x);
}

// log10 -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_slog10(float x)
{
    return log10f(x);
}

LFORTRAN_API double _lfortran_dlog10(double x)
{
    return log10(x);
}

// gamma -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_sgamma(float x)
{
    return tgammaf(x);
}

LFORTRAN_API double _lfortran_dgamma(double x)
{
    return tgamma(x);
}

// gamma -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_slog_gamma(float x)
{
    return lgammaf(x);
}

LFORTRAN_API double _lfortran_dlog_gamma(double x)
{
    return lgamma(x);
}

// sin -------------------------------------------------------------------------

LFORTRAN_API float _lfortran_ssin(float x)
{
    return sinf(x);
}

LFORTRAN_API double _lfortran_dsin(double x)
{
    return sin(x);
}

LFORTRAN_API float_complex_t _lfortran_csin(float_complex_t x)
{
    return csinf(x);
}

LFORTRAN_API double_complex_t _lfortran_zsin(double_complex_t x)
{
    return csin(x);
}

// cos -------------------------------------------------------------------------

LFORTRAN_API float _lfortran_scos(float x)
{
    return cosf(x);
}

LFORTRAN_API double _lfortran_dcos(double x)
{
    return cos(x);
}

LFORTRAN_API float_complex_t _lfortran_ccos(float_complex_t x)
{
    return ccosf(x);
}

LFORTRAN_API double_complex_t _lfortran_zcos(double_complex_t x)
{
    return ccos(x);
}

// tan -------------------------------------------------------------------------

LFORTRAN_API float _lfortran_stan(float x)
{
    return tanf(x);
}

LFORTRAN_API double _lfortran_dtan(double x)
{
    return tan(x);
}

LFORTRAN_API float_complex_t _lfortran_ctan(float_complex_t x)
{
    return ctanf(x);
}

LFORTRAN_API double_complex_t _lfortran_ztan(double_complex_t x)
{
    return ctan(x);
}

// sinh ------------------------------------------------------------------------

LFORTRAN_API float _lfortran_ssinh(float x)
{
    return sinhf(x);
}

LFORTRAN_API double _lfortran_dsinh(double x)
{
    return sinh(x);
}

LFORTRAN_API float_complex_t _lfortran_csinh(float_complex_t x)
{
    return csinhf(x);
}

LFORTRAN_API double_complex_t _lfortran_zsinh(double_complex_t x)
{
    return csinh(x);
}

// cosh ------------------------------------------------------------------------


LFORTRAN_API float _lfortran_scosh(float x)
{
    return coshf(x);
}

LFORTRAN_API double _lfortran_dcosh(double x)
{
    return cosh(x);
}

LFORTRAN_API float_complex_t _lfortran_ccosh(float_complex_t x)
{
    return ccoshf(x);
}

LFORTRAN_API double_complex_t _lfortran_zcosh(double_complex_t x)
{
    return ccosh(x);
}

// tanh ------------------------------------------------------------------------

LFORTRAN_API float _lfortran_stanh(float x)
{
    return tanhf(x);
}

LFORTRAN_API double _lfortran_dtanh(double x)
{
    return tanh(x);
}

LFORTRAN_API float_complex_t _lfortran_ctanh(float_complex_t x)
{
    return ctanhf(x);
}

LFORTRAN_API double_complex_t _lfortran_ztanh(double_complex_t x)
{
    return ctanh(x);
}

// asin ------------------------------------------------------------------------

LFORTRAN_API float _lfortran_sasin(float x)
{
    return asinf(x);
}

LFORTRAN_API double _lfortran_dasin(double x)
{
    return asin(x);
}

LFORTRAN_API float_complex_t _lfortran_casin(float_complex_t x)
{
    return casinf(x);
}

LFORTRAN_API double_complex_t _lfortran_zasin(double_complex_t x)
{
    return casin(x);
}

// acos ------------------------------------------------------------------------

LFORTRAN_API float _lfortran_sacos(float x)
{
    return acosf(x);
}

LFORTRAN_API double _lfortran_dacos(double x)
{
    return acos(x);
}

LFORTRAN_API float_complex_t _lfortran_cacos(float_complex_t x)
{
    return cacosf(x);
}

LFORTRAN_API double_complex_t _lfortran_zacos(double_complex_t x)
{
    return cacos(x);
}

// atan ------------------------------------------------------------------------

LFORTRAN_API float _lfortran_satan(float x)
{
    return atanf(x);
}

LFORTRAN_API double _lfortran_datan(double x)
{
    return atan(x);
}

LFORTRAN_API float_complex_t _lfortran_catan(float_complex_t x)
{
    return catanf(x);
}

LFORTRAN_API double_complex_t _lfortran_zatan(double_complex_t x)
{
    return catan(x);
}

// atan2 -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_satan2(float y, float x)
{
    return atan2f(y, x);
}

LFORTRAN_API double _lfortran_datan2(double y, double x)
{
    return atan2(y, x);
}

// asinh -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_sasinh(float x)
{
    return asinhf(x);
}

LFORTRAN_API double _lfortran_dasinh(double x)
{
    return asinh(x);
}

LFORTRAN_API float_complex_t _lfortran_casinh(float_complex_t x)
{
    return casinhf(x);
}

LFORTRAN_API double_complex_t _lfortran_zasinh(double_complex_t x)
{
    return casinh(x);
}

// acosh -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_sacosh(float x)
{
    return acoshf(x);
}

LFORTRAN_API double _lfortran_dacosh(double x)
{
    return acosh(x);
}

LFORTRAN_API float_complex_t _lfortran_cacosh(float_complex_t x)
{
    return cacoshf(x);
}

LFORTRAN_API double_complex_t _lfortran_zacosh(double_complex_t x)
{
    return cacosh(x);
}

// fmod -----------------------------------------------------------------------

LFORTRAN_API double _lfortran_dfmod(double x, double y)
{
    return fmod(x, y);
}

// atanh -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_satanh(float x)
{
    return atanhf(x);
}

LFORTRAN_API double _lfortran_datanh(double x)
{
    return atanh(x);
}

LFORTRAN_API float_complex_t _lfortran_catanh(float_complex_t x)
{
    return catanhf(x);
}

LFORTRAN_API double_complex_t _lfortran_zatanh(double_complex_t x)
{
    return catanh(x);
}

// phase --------------------------------------------------------------------

LFORTRAN_API float _lfortran_cphase(float_complex_t x)
{
    return atan2f(cimagf(x), crealf(x));
}

LFORTRAN_API double _lfortran_zphase(double_complex_t x)
{
    return atan2(cimag(x), creal(x));
}

// strcat  --------------------------------------------------------------------

LFORTRAN_API void _lfortran_strcat(char** s1, char** s2, char** dest)
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

// strcpy -----------------------------------------------------------

LFORTRAN_API void _lfortran_strcpy(char** x, char *y, int8_t free_target)
{
    if (*x && free_target) free((void *)*x);
    *x = (char*) malloc((strlen(y) + 1) * sizeof(char));
    strcpy(*x, y);
}

#define MIN(x, y) ((x < y) ? x : y)

int str_compare(char **s1, char **s2)
{
    int s1_len = strlen(*s1);
    int s2_len = strlen(*s2);
    int lim = MIN(s1_len, s2_len);
    int res = 0;
    int i ;
    for (i = 0; i < lim; i++) {
        if ((*s1)[i] != (*s2)[i]) {
            res = (*s1)[i] - (*s2)[i];
            break;
        }
    }
    res = (i == lim)? s1_len - s2_len : res;
    return res;
}
LFORTRAN_API bool _lpython_str_compare_eq(char **s1, char **s2)
{
    return str_compare(s1, s2) == 0;
}

LFORTRAN_API bool _lpython_str_compare_noteq(char **s1, char **s2)
{
    return str_compare(s1, s2) != 0;
}

LFORTRAN_API bool _lpython_str_compare_gt(char **s1, char **s2)
{
    return str_compare(s1, s2) > 0;
}

LFORTRAN_API bool _lpython_str_compare_lte(char **s1, char **s2)
{
    return str_compare(s1, s2) <= 0;
}

LFORTRAN_API bool _lpython_str_compare_lt(char **s1, char **s2)
{
    return str_compare(s1, s2) < 0;
}

LFORTRAN_API bool _lpython_str_compare_gte(char **s1, char **s2)
{
    return str_compare(s1, s2) >= 0;
}

LFORTRAN_API char* _lfortran_float_to_str4(float num)
{
    char* res = (char*)malloc(40);
    sprintf(res, "%f", num);
    return res;
}

LFORTRAN_API char* _lfortran_float_to_str8(double num)
{
    char* res = (char*)malloc(40);
    sprintf(res, "%f", num);
    return res;
}

LFORTRAN_API char* _lfortran_int_to_str1(int8_t num)
{
    char* res = (char*)malloc(40);
    sprintf(res, "%d", num);
    return res;
}

LFORTRAN_API char* _lfortran_int_to_str2(int16_t num)
{
    char* res = (char*)malloc(40);
    sprintf(res, "%d", num);
    return res;
}

LFORTRAN_API char* _lfortran_int_to_str4(int32_t num)
{
    char* res = (char*)malloc(40);
    sprintf(res, "%d", num);
    return res;
}

LFORTRAN_API char* _lfortran_int_to_str8(int64_t num)
{
    char* res = (char*)malloc(40);
    long long num2 = num;
    sprintf(res, "%lld", num2);
    return res;
}

LFORTRAN_API int32_t _lpython_bit_length1(int8_t num)
{
    int32_t res = 0;
    num = abs((int)num);
    while (num > 0) {
        num = num >> 1;
        res++;
    }
    return res;
}

LFORTRAN_API int32_t _lpython_bit_length2(int16_t num)
{
    int32_t res = 0;
    num = abs((int)num);
    while (num > 0) {
        num = num >> 1;
        res++;
    }
    return res;
}

LFORTRAN_API int32_t _lpython_bit_length4(int32_t num)
{
    int32_t res = 0;
    num = abs((int)num);
    while (num > 0) {
        num = num >> 1;
        res++;
    }
    return res;
}

LFORTRAN_API int32_t _lpython_bit_length8(int64_t num)
{
    int32_t res = 0;
    num = llabs(num);
    while (num > 0) {
        num = num >> 1;
        res++;
    }
    return res;
}

//repeat str for n time
LFORTRAN_API void _lfortran_strrepeat(char** s, int32_t n, char** dest)
{
    int cntr = 0;
    char trmn = '\0';
    int s_len = strlen(*s);
    int trmn_size = sizeof(trmn);
    int f_len = s_len*n;
    if (f_len < 0)
        f_len = 0;
    char* dest_char = (char*)malloc(f_len+trmn_size);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < s_len; j++) {
            dest_char[cntr] = (*s)[j];
            cntr++;
        }
    }
    dest_char[cntr] = trmn;
    *dest = &(dest_char[0]);
}

LFORTRAN_API char* _lfortran_strrepeat_c(char* s, int32_t n)
{
    int cntr = 0;
    char trmn = '\0';
    int s_len = strlen(s);
    int trmn_size = sizeof(trmn);
    int f_len = s_len*n;
    if (f_len < 0)
        f_len = 0;
    char* dest_char = (char*)malloc(f_len+trmn_size);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < s_len; j++) {
            dest_char[cntr] = s[j];
            cntr++;
        }
    }
    dest_char[cntr] = trmn;
    return dest_char;
}

// idx starts from 1
LFORTRAN_API char* _lfortran_str_item(char* s, int32_t idx) {

    int s_len = strlen(s);
    int original_idx = idx - 1;
    if (idx < 1) idx += s_len;
    if (idx < 1 || idx >= s_len + 1) {
        printf("String index: %d is out of Bounds\n", original_idx);
        exit(1);
    }
    char* res = (char*)malloc(2);
    res[0] = s[idx-1];
    res[1] = '\0';
    return res;
}

// idx1 and idx2 both start from 1
LFORTRAN_API char* _lfortran_str_copy(char* s, int32_t idx1, int32_t idx2) {

    int s_len = strlen(s);
    if(idx1 > s_len || idx1 <= (-1*s_len)){
        printf("String index out of Bounds\n");
        exit(1);
    }
    if(idx1 <= 0) {
        idx1 = s_len + idx1;
    }
    if(idx2 <= 0) {
        idx2 = s_len + idx2;
    }
    char* dest_char = (char*)malloc(idx2-idx1+2);
    for (int i=idx1; i <= idx2; i++)
    {
        dest_char[i-idx1] = s[i-1];
    }
    dest_char[idx2-idx1+1] = '\0';
    return dest_char;
}

LFORTRAN_API char* _lfortran_str_slice(char* s, int32_t idx1, int32_t idx2, int32_t step,
                        bool idx1_present, bool idx2_present) {
    int s_len = strlen(s);
    if (step == 0) {
        printf("slice step cannot be zero\n");
        exit(1);
    }
    idx1 = idx1 < 0 ? idx1 + s_len : idx1;
    idx2 = idx2 < 0 ? idx2 + s_len : idx2;
    if (!idx1_present) {
        if (step > 0) {
            idx1 = 0;
        } else {
            idx1 = s_len - 1;
        }
    }
    if (!idx2_present) {
        if (step > 0) {
            idx2 = s_len;
        } else {
            idx2 = -1;
        }
    }
    if (idx1 == idx2 ||
        (step > 0 && (idx1 > idx2 || idx1 >= s_len)) ||
        (step < 0 && (idx1 < idx2 || idx2 >= s_len-1)))
        return "";
    int dest_len = 0;
    if (step > 0) {
        idx2 = idx2 > s_len ? s_len : idx2;
        dest_len = (idx2-idx1+step-1)/step + 1;
    } else {
        idx1 = idx1 >= s_len ? s_len-1 : idx1;
        dest_len = (idx2-idx1+step+1)/step + 1;
    }

    char* dest_char = (char*)malloc(dest_len);
    int s_i = idx1, d_i = 0;
    while((step > 0 && s_i >= idx1 && s_i < idx2) ||
        (step < 0 && s_i <= idx1 && s_i > idx2)) {
        dest_char[d_i++] = s[s_i];
        s_i+=step;
    }
    dest_char[d_i] = '\0';
    return dest_char;
}

LFORTRAN_API char* _lfortran_str_slice_assign(char* s, char *r, int32_t idx1, int32_t idx2, int32_t step,
                        bool idx1_present, bool idx2_present) {
    int s_len = strlen(s);
    int r_len = strlen(r);
    if (step == 0) {
        printf("slice step cannot be zero\n");
        exit(1);
    }
    s_len = (s_len < r_len) ? r_len : s_len;
    idx1 = idx1 < 0 ? idx1 + s_len : idx1;
    idx2 = idx2 < 0 ? idx2 + s_len : idx2;
    if (!idx1_present) {
        if (step > 0) {
            idx1 = 0;
        } else {
            idx1 = s_len - 1;
        }
    }
    if (!idx2_present) {
        if (step > 0) {
            idx2 = s_len;
        } else {
            idx2 = -1;
        }
    }
    if (idx1 == idx2 ||
        (step > 0 && (idx1 > idx2 || idx1 >= s_len)) ||
        (step < 0 && (idx1 < idx2 || idx2 >= s_len-1)))
        return "";
    char* dest_char = (char*)malloc(s_len);
    strcpy(dest_char, s);
    int s_i = idx1, d_i = 0;
    while((step > 0 && s_i >= idx1 && s_i < idx2) ||
        (step < 0 && s_i <= idx1 && s_i > idx2)) {
        dest_char[s_i] = r[d_i++];
        s_i += step;
    }
    return dest_char;
}

LFORTRAN_API int32_t _lfortran_str_len(char** s)
{
    return strlen(*s);
}

LFORTRAN_API int _lfortran_str_to_int(char** s)
{
    char *ptr;
    return strtol(*s, &ptr, 10);
}

LFORTRAN_API int _lfortran_str_ord(char** s)
{
    return (*s)[0];
}

LFORTRAN_API int _lfortran_str_ord_c(char* s)
{
    return s[0];
}

LFORTRAN_API char* _lfortran_str_chr(int val)
{
    char* dest_char = (char*)malloc(2);
    dest_char[0] = val;
    dest_char[1] = '\0';
    return dest_char;
}

LFORTRAN_API void _lfortran_memset(void* s, int32_t c, int32_t size) {
    memset(s, c, size);
}

LFORTRAN_API void* _lfortran_malloc(int32_t size) {
    return malloc(size);
}

LFORTRAN_API int8_t* _lfortran_realloc(int8_t* ptr, int32_t size) {
    return (int8_t*) realloc(ptr, size);
}

LFORTRAN_API int8_t* _lfortran_calloc(int32_t count, int32_t size) {
    return (int8_t*) calloc(count, size);
}

LFORTRAN_API void _lfortran_free(char* ptr) {
    free((void*)ptr);
}

LFORTRAN_API void _lfortran_alloc(char** ptr, int32_t size) {
    *ptr = (char *) malloc(size);
}

// size_plus_one is the size of the string including the null character
LFORTRAN_API void _lfortran_string_init(int size_plus_one, char *s) {
    int size = size_plus_one-1;
    for (int i=0; i < size; i++) {
        s[i] = ' ';
    }
    s[size] = '\0';
}

// bit  ------------------------------------------------------------------------

LFORTRAN_API int32_t _lfortran_iand32(int32_t x, int32_t y) {
    return x & y;
}

LFORTRAN_API int64_t _lfortran_iand64(int64_t x, int64_t y) {
    return x & y;
}

LFORTRAN_API int32_t _lfortran_not32(int32_t x) {
    return ~ x;
}

LFORTRAN_API int64_t _lfortran_not64(int64_t x) {
    return ~ x;
}

LFORTRAN_API int32_t _lfortran_ior32(int32_t x, int32_t y) {
    return x | y;
}

LFORTRAN_API int64_t _lfortran_ior64(int64_t x, int64_t y) {
    return x | y;
}

LFORTRAN_API int32_t _lfortran_ieor32(int32_t x, int32_t y) {
    return x ^ y;
}

LFORTRAN_API int64_t _lfortran_ieor64(int64_t x, int64_t y) {
    return x ^ y;
}

LFORTRAN_API int32_t _lfortran_ibclr32(int32_t i, int pos) {
    return i & ~(1 << pos);
}

LFORTRAN_API int64_t _lfortran_ibclr64(int64_t i, int pos) {
    return i & ~(1LL << pos);
}

LFORTRAN_API int32_t _lfortran_ibset32(int32_t i, int pos) {
    return i | (1 << pos);
}

LFORTRAN_API int64_t _lfortran_ibset64(int64_t i, int pos) {
    return i | (1LL << pos);
}

LFORTRAN_API int32_t _lfortran_btest32(int32_t i, int pos) {
    return i & (1 << pos);
}

LFORTRAN_API int64_t _lfortran_btest64(int64_t i, int pos) {
    return i & (1LL << pos);
}

LFORTRAN_API int32_t _lfortran_ishft32(int32_t i, int32_t shift) {
    if(shift > 0) {
        return i << shift;
    } else if(shift < 0) {
        return i >> abs(shift);
    } else {
        return i;
    }
}

LFORTRAN_API int64_t _lfortran_ishft64(int64_t i, int64_t shift) {
    if(shift > 0) {
        return i << shift;
    } else if(shift < 0) {
        return i >> llabs(shift);
    } else {
        return i;
    }
}

LFORTRAN_API int32_t _lfortran_mvbits32(int32_t from, int32_t frompos,
                                        int32_t len, int32_t to, int32_t topos) {
    uint32_t all_ones = ~0;
    uint32_t ufrom = from;
    uint32_t uto = to;
    all_ones <<= (BITS_32 - frompos - len);
    all_ones >>= (BITS_32 - len);
    all_ones <<= topos;
    ufrom <<= (BITS_32 - frompos - len);
    ufrom >>= (BITS_32 - len);
    ufrom <<= topos;
    return (~all_ones & uto) | ufrom;
}

LFORTRAN_API int64_t _lfortran_mvbits64(int64_t from, int32_t frompos,
                                        int32_t len, int64_t to, int32_t topos) {
    uint64_t all_ones = ~0;
    uint64_t ufrom = from;
    uint64_t uto = to;
    all_ones <<= (BITS_64 - frompos - len);
    all_ones >>= (BITS_64 - len);
    all_ones <<= topos;
    ufrom <<= (BITS_64 - frompos - len);
    ufrom >>= (BITS_64 - len);
    ufrom <<= topos;
    return (~all_ones & uto) | ufrom;
}

LFORTRAN_API int32_t _lfortran_bgt32(int32_t i, int32_t j) {
    uint32_t ui = i, uj = j;
    return ui > uj;
}

LFORTRAN_API int32_t _lfortran_bgt64(int64_t i, int64_t j) {
    uint64_t ui = i, uj = j;
    return ui > uj;
}

LFORTRAN_API int32_t _lfortran_bge32(int32_t i, int32_t j) {
    uint32_t ui = i, uj = j;
    return ui >= uj;
}

LFORTRAN_API int32_t _lfortran_bge64(int64_t i, int64_t j) {
    uint64_t ui = i, uj = j;
    return ui >= uj;
}

LFORTRAN_API int32_t _lfortran_ble32(int32_t i, int32_t j) {
    uint32_t ui = i, uj = j;
    return ui <= uj;
}

LFORTRAN_API int32_t _lfortran_ble64(int64_t i, int64_t j) {
    uint64_t ui = i, uj = j;
    return ui <= uj;
}

LFORTRAN_API int32_t _lfortran_blt32(int32_t i, int32_t j) {
    uint32_t ui = i, uj = j;
    return ui < uj;
}

LFORTRAN_API int32_t _lfortran_blt64(int64_t i, int64_t j) {
    uint64_t ui = i, uj = j;
    return ui < uj;
}

LFORTRAN_API int32_t _lfortran_ibits32(int32_t i, int32_t pos, int32_t len) {
    uint32_t ui = i;
    return ((ui << (BITS_32 - pos - len)) >> (BITS_32 - len));
}

LFORTRAN_API int64_t _lfortran_ibits64(int64_t i, int32_t pos, int32_t len) {
    uint64_t ui = i;
    return ((ui << (BITS_64 - pos - len)) >> (BITS_64 - len));
}

// cpu_time  -------------------------------------------------------------------

LFORTRAN_API void _lfortran_cpu_time(double *t) {
    *t = ((double) clock()) / CLOCKS_PER_SEC;
}

// system_time -----------------------------------------------------------------

LFORTRAN_API void _lfortran_i32sys_clock(
        int32_t *count, int32_t *rate, int32_t *max) {
#if defined(_MSC_VER) || defined(__MACH__)
        *count = - INT_MAX;
        *rate = 0;
        *max = 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        *count = (int32_t)(ts.tv_nsec / 1000000) + ((int32_t)ts.tv_sec * 1000);
        *rate = 1e3; // milliseconds
        *max = INT_MAX;
    } else {
        *count = - INT_MAX;
        *rate = 0;
        *max = 0;
    }
#endif
}

LFORTRAN_API void _lfortran_i64sys_clock(
        uint64_t *count, int64_t *rate, int64_t *max) {
#if defined(_MSC_VER) || defined(__MACH__)
        *count = - INT_MAX;
        *rate = 0;
        *max = 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        *count = (uint64_t)(ts.tv_nsec) + ((uint64_t)ts.tv_sec * 1000000000);
        // FIXME: Rate can be in microseconds or nanoseconds depending on
        //          resolution of the underlying platform clock.
        *rate = 1e9; // nanoseconds
        *max = LLONG_MAX;
    } else {
        *count = - LLONG_MAX;
        *rate = 0;
        *max = 0;
    }
#endif
}

LFORTRAN_API double _lfortran_time()
{
#if defined(_MSC_VER)
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (double)uli.QuadPart / 10000000.0 - 11644473600.0;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

LFORTRAN_API void _lfortran_sp_rand_num(float *x) {
    srand(time(0));
    *x = rand() / (float) RAND_MAX;
}

LFORTRAN_API void _lfortran_dp_rand_num(double *x) {
    srand(time(0));
    *x = rand() / (double) RAND_MAX;
}

LFORTRAN_API int64_t _lpython_open(char *path, char *flags)
{
    FILE *fd;
    fd = fopen(path, flags);
    if (!fd)
    {
        printf("Error in opening the file!\n");
        perror(path);
        exit(1);
    }
    return (int64_t)fd;
}

#define MAXUNITS 100

FILE* unit_to_file[MAXUNITS];
bool is_unit_to_file_init = false;

LFORTRAN_API int64_t _lfortran_open(int32_t unit_num, char *f_name, char *status)
{
    if (!is_unit_to_file_init) {
        for (int32_t i=0; i<100; i++) unit_to_file[i] = NULL;
        is_unit_to_file_init = true;
    }
    if (f_name == NULL) {
        f_name = "_lfortran_generated_file.txt";
    }

    // Presently we just consider write append mode.
    status = "a+";
    FILE *fd;
    fd = fopen(f_name, status);
    if (!fd)
    {
        printf("Error in opening the file!\n");
        perror(f_name);
        exit(1);
    }
    unit_to_file[unit_num] = fd;
    return (int64_t)fd;
}

LFORTRAN_API void _lfortran_flush(int32_t unit_num)
{
    if( !is_unit_to_file_init || unit_to_file[unit_num] == NULL ) {
        printf("Specified UNIT %d in FLUSH is not connected.\n", unit_num);
        exit(1);
    }
    fflush(unit_to_file[unit_num]);
}

LFORTRAN_API void _lfortran_inquire(char *f_name, bool *exists) {
    FILE *fp = fopen(f_name, "r");
    if (fp != NULL) {
        *exists = true;
        fclose(fp); // close the file
        return;
    }
    *exists = false;
}

LFORTRAN_API void _lfortran_rewind(int32_t unit_num)
{
    if( !is_unit_to_file_init || unit_to_file[unit_num] == NULL ) {
        printf("Specified UNIT %d in REWIND is not created or connected.\n", unit_num);
        exit(1);
    }
    rewind(unit_to_file[unit_num]);
}


LFORTRAN_API void _lfortran_read_int32(int32_t *p, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        FILE *fp = fdopen(0, "r+");
        (void)fread(p, sizeof(int32_t), 1, fp);
        fclose(fp);
        return;
    }
    if (!unit_to_file[unit_num]) {
        printf("No file found with given unit\n");
        exit(1);
    }
    (void)fread(p, sizeof(int32_t), 1, unit_to_file[unit_num]);
}

LFORTRAN_API void _lfortran_read_char(char **p, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        *p = (char*)malloc(16);
        FILE *fp = fdopen(0, "r+");
        (void)fread(*p, sizeof(char), 16, fp);
        fclose(fp);
        return;
    }
    if (!unit_to_file[unit_num]) {
        printf("No file found with given unit\n");
        exit(1);
    }
    *p = (char*)malloc(16);
    (void)fread(*p, sizeof(char), 16, unit_to_file[unit_num]);
}

LFORTRAN_API char* _lpython_read(int64_t fd, int64_t n)
{
    char *c = (char *) calloc(n, sizeof(char));
    if (fd < 0)
    {
        printf("Error in reading the file!\n");
        exit(1);
    }
    int x = fread(c, 1, n, (FILE*)fd);
    c[x] = '\0';
    return c;
}

LFORTRAN_API void _lpython_close(int64_t fd)
{
    if (fclose((FILE*)fd) != 0)
    {
        printf("Error in closing the file!\n");
        exit(1);
    }
}

LFORTRAN_API void _lfortran_close(int32_t unit_num)
{
    if (!unit_to_file[unit_num]) {
        printf("No file found with given unit\n");
        exit(1);
    }
    if (fclose(unit_to_file[unit_num]) != 0)
    {
        printf("Error in closing the file!\n");
        exit(1);
    }
}

LFORTRAN_API int32_t _lfortran_ichar(char *c) {
     return (int32_t) c[0];
}

LFORTRAN_API int32_t _lfortran_iachar(char *c) {
    return (int32_t) c[0];
}

LFORTRAN_API int32_t _lfortran_all(bool *mask, int32_t n) {
    for (size_t i = 0; i < n; i++) {
        if (!mask[i]) {
            return false;
        }
    }
    return true;
}

// Command line arguments
int32_t _argc;
char **_argv;

LFORTRAN_API void _lpython_set_argv(int32_t argc_1, char *argv_1[]) {
    _argv = malloc(argc_1 * sizeof(char *));
    for (size_t i = 0; i < argc_1; i++) {
        _argv[i] = strdup(argv_1[i]);
    }
    _argc = argc_1;
}

LFORTRAN_API int32_t _lpython_get_argc() {
    return _argc;
}

LFORTRAN_API char *_lpython_get_argv(int32_t index) {
    return _argv[index];
}

// << Command line arguments << ------------------------------------------------

// >> Runtime Stacktrace >> ----------------------------------------------------
#ifdef HAVE_RUNTIME_STACKTRACE
#ifdef HAVE_LFORTRAN_UNWIND
static _Unwind_Reason_Code unwind_callback(struct _Unwind_Context *context,
        void *vdata) {
    struct Stacktrace *d = (struct Stacktrace *) vdata;
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc != 0) {
        pc--;
        if (d->pc_size < LCOMPILERS_MAX_STACKTRACE_LENGTH) {
            d->pc[d->pc_size] = pc;
            d->pc_size++;
        } else {
            printf("The stacktrace length is out of range.\nAborting...");
            abort();
        }
    }
    return _URC_NO_REASON;
}
#endif // HAVE_LFORTRAN_UNWIND

struct Stacktrace get_stacktrace_addresses() {
    struct Stacktrace d;
    d.pc_size = 0;
#ifdef HAVE_LFORTRAN_UNWIND
    _Unwind_Backtrace(unwind_callback, &d);
#endif
    return d;
}

char *get_base_name(char *filename) {
    size_t start = strrchr(filename, '/')-filename+1;
    size_t end = strrchr(filename, '.')-filename-1;
    int nos_of_chars = end - start + 1;
    char *base_name;
    if (nos_of_chars < 0) {
        return NULL;
    }
    base_name = malloc (sizeof (char) * (nos_of_chars + 1));
    base_name[nos_of_chars] = '\0';
    strncpy (base_name, filename + start, nos_of_chars);
    return base_name;
}

#ifdef HAVE_LFORTRAN_LINK
int shared_lib_callback(struct dl_phdr_info *info,
        size_t size, void *_data) {
    struct Stacktrace *d = (struct Stacktrace *) _data;
    for (int i = 0; i < info->dlpi_phnum; i++) {
        if (info->dlpi_phdr[i].p_type == PT_LOAD) {
            ElfW(Addr) min_addr = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr;
            ElfW(Addr) max_addr = min_addr + info->dlpi_phdr[i].p_memsz;
            if ((d->current_pc >= min_addr) && (d->current_pc < max_addr)) {
                d->binary_filename[d->local_pc_size] = (char *)info->dlpi_name;
                if (d->binary_filename[d->local_pc_size][0] == '\0') {
                    d->binary_filename[d->local_pc_size] = binary_executable_path;
                    d->local_pc[d->local_pc_size] = d->current_pc - info->dlpi_addr;
                    d->local_pc_size++;
                }
                // We found a match, return a non-zero value
                return 1;
            }
        }
    }
    // We didn't find a match, return a zero value
    return 0;
}
#endif // HAVE_LFORTRAN_LINK

#ifdef HAVE_LFORTRAN_MACHO
void get_local_address_mac(struct Stacktrace *d) {
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const struct mach_header *header = _dyld_get_image_header(i);
        intptr_t offset = _dyld_get_image_vmaddr_slide(i);
        struct load_command* cmd = (struct load_command*)((char *)header + sizeof(struct mach_header));
        if(header->magic == MH_MAGIC_64) {
            cmd = (struct load_command*)((char *)header + sizeof(struct mach_header_64));
        }
        for (uint32_t j = 0; j < header->ncmds; j++) {
            if (cmd->cmd == LC_SEGMENT) {
                struct segment_command* seg = (struct segment_command*)cmd;
                if (((intptr_t)d->current_pc >= (seg->vmaddr+offset)) &&
                    ((intptr_t)d->current_pc < (seg->vmaddr+offset + seg->vmsize))) {
                    int check_filename = strcmp(get_base_name(
                        (char *)_dyld_get_image_name(i)),
                        get_base_name(source_filename));
                    if ( check_filename != 0 ) return;
                    d->local_pc[d->local_pc_size] = d->current_pc - offset;
                    d->binary_filename[d->local_pc_size] = (char *)_dyld_get_image_name(i);
                    // Resolve symlinks to a real path:
                    char buffer[PATH_MAX];
                    char* resolved;
                    resolved = realpath(d->binary_filename[d->local_pc_size], buffer);
                    if (resolved) d->binary_filename[d->local_pc_size] = resolved;
                    d->local_pc_size++;
                    return;
                }
            }
            if (cmd->cmd == LC_SEGMENT_64) {
                struct segment_command_64* seg = (struct segment_command_64*)cmd;
                if ((d->current_pc >= (seg->vmaddr + offset)) &&
                    (d->current_pc < (seg->vmaddr + offset + seg->vmsize))) {
                    int check_filename = strcmp(get_base_name(
                        (char *)_dyld_get_image_name(i)),
                        get_base_name(source_filename));
                    if ( check_filename != 0 ) return;
                    d->local_pc[d->local_pc_size] = d->current_pc - offset;
                    d->binary_filename[d->local_pc_size] = (char *)_dyld_get_image_name(i);
                    // Resolve symlinks to a real path:
                    char buffer[PATH_MAX];
                    char* resolved;
                    resolved = realpath(d->binary_filename[d->local_pc_size], buffer);
                    if (resolved) d->binary_filename[d->local_pc_size] = resolved;
                    d->local_pc_size++;
                    return;
                }
            }
            cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
        }
    }
    printf("The stack address was not found in any shared library or"
        " the main program, the stack is probably corrupted.\n"
        "Aborting...\n");
    abort();
}
#endif // HAVE_LFORTRAN_MACHO

// Fills in `local_pc` and `binary_filename`
void get_local_address(struct Stacktrace *d) {
    d->local_pc_size = 0;
    for (int32_t i=0; i < d->pc_size; i++) {
        d->current_pc = d->pc[i];
#ifdef HAVE_LFORTRAN_LINK
        // Iterates over all loaded shared libraries
        // See `stacktrace.cpp` to get more information
        if (dl_iterate_phdr(shared_lib_callback, d) == 0) {
            printf("The stack address was not found in any shared library or"
                " the main program, the stack is probably corrupted.\n"
                "Aborting...\n");
            abort();
        }
#else
#ifdef HAVE_LFORTRAN_MACHO
        get_local_address_mac(& *d);
#else
    d->local_pc[d->local_pc_size] = 0;
    d->local_pc_size++;
#endif // HAVE_LFORTRAN_MACHO
#endif // HAVE_LFORTRAN_LINK
    }
}

uint32_t get_file_size(int64_t fp) {
    FILE *fp_ = (FILE *)fp;
    int prev = ftell(fp_);
    fseek(fp_, 0, SEEK_END);
    int size = ftell(fp_);
    fseek(fp_, prev, SEEK_SET);
    return size;
}

/*
 * `lines_dat.txt` file must be created before calling this function,
 * The file can be created using the command:
 *     ./src/bin/dat_convert.py lines.dat
 * This function fills in the `addresses` and `line_numbers`
 * from the `lines_dat.txt` file.
 */
void get_local_info_dwarfdump(struct Stacktrace *d) {
    // TODO: Read the contents of lines.dat from here itself.
    char *base_name = get_base_name(source_filename);
    char *filename = malloc(strlen(base_name) + 14);
    strcpy(filename, base_name);
    strcat(filename, "_lines.dat.txt");
    int64_t fd = _lpython_open(filename, "r");
    uint32_t size = get_file_size(fd);
    char *file_contents = _lpython_read(fd, size);
    _lpython_close(fd);
    free(filename);

    char s[LCOMPILERS_MAX_STACKTRACE_LENGTH];
    bool address = true;
    uint32_t j = 0;
    d->stack_size = 0;
    for (uint32_t i = 0; i < size; i++) {
        if (file_contents[i] == '\n') {
            memset(s, '\0', sizeof(s));
            j = 0;
            d->stack_size++;
            continue;
        } else if (file_contents[i] == ' ') {
            s[j] = '\0';
            j = 0;
            if (address) {
                d->addresses[d->stack_size] = strtol(s, NULL, 10);
                address = false;
            } else {
                d->line_numbers[d->stack_size] = strtol(s, NULL, 10);
                address = true;
            }
            memset(s, '\0', sizeof(s));
            continue;
        }
        s[j++] = file_contents[i];
    }
}

char *read_line_from_file(char *filename, uint32_t line_number) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0, n = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) exit(1);
    while (n < line_number && (getline(&line, &len, fp) != -1)) n++;
    fclose(fp);

    return line;
}

static inline uint64_t bisection(const uint64_t vec[],
        uint64_t size, uint64_t i) {
    if (i < vec[0]) return 0;
    if (i >= vec[size-1]) return size;
    uint64_t i1 = 0, i2 = size-1;
    while (i1 < i2-1) {
        uint64_t imid = (i1+i2)/2;
        if (i < vec[imid]) {
            i2 = imid;
        } else {
            i1 = imid;
        }
    }
    return i1;
}

char *remove_whitespace(char *str) {
    if (str == NULL || str[0] == '\0') {
        return "(null)";
    }
    char *end;
    // remove leading space
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) // All spaces?
        return str;
    // remove trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    // Write new null terminator character
    end[1] = '\0';
    return str;
}
#endif // HAVE_RUNTIME_STACKTRACE

LFORTRAN_API void print_stacktrace_addresses(char *filename, bool use_colors) {
#ifdef HAVE_RUNTIME_STACKTRACE
    source_filename = filename;
    struct Stacktrace d = get_stacktrace_addresses();
    get_local_address(&d);
    get_local_info_dwarfdump(&d);

#ifdef HAVE_LFORTRAN_MACHO
    for (int32_t i = d.local_pc_size-2; i >= 0; i--) {
#else
    for (int32_t i = d.local_pc_size-3; i >= 0; i--) {
#endif
        uint64_t index = bisection(d.addresses, d.stack_size, d.local_pc[i]);
        if(use_colors) {
            fprintf(stderr, DIM "  File " S_RESET
                BOLD MAGENTA "\"%s\"" C_RESET S_RESET
#ifdef HAVE_LFORTRAN_MACHO
                DIM ", line %lld\n" S_RESET
#else
                DIM ", line %ld\n" S_RESET
#endif
                "    %s\n", source_filename, d.line_numbers[index],
                remove_whitespace(read_line_from_file(source_filename,
                d.line_numbers[index])));
        } else {
            fprintf(stderr, "  File \"%s\", "
#ifdef HAVE_LFORTRAN_MACHO
                "line %lld\n    %s\n",
#else
                "line %ld\n    %s\n",
#endif
                source_filename, d.line_numbers[index],
                remove_whitespace(read_line_from_file(source_filename,
                d.line_numbers[index])));
        }
#ifdef HAVE_LFORTRAN_MACHO
    }
#else
    }
#endif
#endif // HAVE_RUNTIME_STACKTRACE
}

// << Runtime Stacktrace << ----------------------------------------------------

LFORTRAN_API char *_lfortran_get_env_variable(char *name) {
    return getenv(name);
}

LFORTRAN_API int _lfortran_exec_command(char *cmd) {
    return system(cmd);
}
