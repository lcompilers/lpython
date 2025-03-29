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

#define PI 3.14159265358979323846
#if defined(_WIN32)
#  include <winsock2.h>
#  include <io.h>
#  define ftruncate _chsize_s
#else
#  include <unistd.h>
#endif

#if defined(__APPLE__)
#  include <sys/time.h>
#endif

#include <libasr/runtime/lfortran_intrinsics.h>
#include <libasr/config.h>

#ifdef HAVE_RUNTIME_STACKTRACE

#ifdef COMPILE_TO_WASM
    #undef HAVE_LFORTRAN_MACHO
    #undef HAVE_LFORTRAN_LINK
#endif

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

#ifdef HAVE_LFORTRAN_MACHO
    #define INT64 "%lld"
#elif HAVE_BUILD_TO_WASM
    #define INT64 "%lld"
#else
    #define INT64 "%ld"
#endif

// This function performs case insensitive string comparison
bool streql(const char *s1, const char* s2) {
#if defined(_MSC_VER)
    return _stricmp(s1, s2) == 0;
#else
    return strcasecmp(s1, s2) == 0;
#endif
}

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

LFORTRAN_API int _lfortran_init_random_seed(unsigned seed)
{
    srand(seed);
    return seed;
}

LFORTRAN_API void _lfortran_init_random_clock()
{
    unsigned int count;
#if defined(_WIN32)
    count = (unsigned int)clock();
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        count = (unsigned int)(ts.tv_nsec);
    } else {
        count = (unsigned int)clock();
    }
#endif
    srand(count);
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
    char* str = va_arg(args, char*);
    char* end = va_arg(args, char*);
    if(str == NULL){
        str = " "; // dummy output
    }
    // Detect "\b" to raise error
    if(str[0] == '\b'){
        str = str+1;
        fprintf(stderr, "%s", str);
        exit(1);
    }
    fprintf(stdout, format, str, end);
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

void handle_integer(char* format, int64_t val, char** result, bool is_signed_plus) {
    int width = 0, min_width = 0;
    char* dot_pos = strchr(format, '.');
    int len;
    int sign_width = (val < 0) ? 1 : 0;
    bool sign_plus_exist = (is_signed_plus && val >= 0);
    if (val == 0) {
        len = 1;
    } else if (val == INT64_MIN) {
        len = 19;
    } else {
        len = (int)log10(llabs(val)) + 1;
    }
    if (dot_pos != NULL) {
        dot_pos++;
        width = atoi(format + 1);
        min_width = atoi(dot_pos);
        if (min_width > width && width != 0) {
            perror("Minimum number of digits cannot be more than the specified width for format.\n");
        }
    } else {
        width = atoi(format + 1);
        if (width == 0) {
            width = len + sign_width + sign_plus_exist;
        }
    }
    if (width >= len + sign_width + sign_plus_exist || width == 0) {
        if (min_width > len) {
            for (int i = 0; i < (width - min_width - sign_width - sign_plus_exist); i++) {
                *result = append_to_string(*result, " ");
            }
            
            if (val < 0) {
                *result = append_to_string(*result, "-");
            } else if(sign_plus_exist){
                *result = append_to_string(*result, "+");
            }

            for (int i = 0; i < (min_width - len); i++) {
                *result = append_to_string(*result, "0");
            }
        } else {
            for (int i = 0; i < (width - len - sign_width - sign_plus_exist); i++) {
                *result = append_to_string(*result, " ");
            }
            if (val < 0) {
                *result = append_to_string(*result, "-");
            } else if (sign_plus_exist){
                *result = append_to_string(*result, "+");
            }
        }
        char str[20];
        if (val == INT64_MIN) {
            sprintf(str, "9223372036854775808");
        } else {
            sprintf(str, "%lld", llabs(val));
        }
        *result = append_to_string(*result, str);
    } else {
        for (int i = 0; i < width; i++) {
            *result = append_to_string(*result, "*");
        }
    }
}

void handle_logical(char* format, bool val, char** result) {
    int width = atoi(format + 1);
    for (int i = 0; i < width - 1; i++) {
        *result = append_to_string(*result, " ");
    }
    if (val) {
        *result = append_to_string(*result, "T");
    } else {
        *result = append_to_string(*result, "F");
    }
}

void handle_float(char* format, double val, char** result, bool use_sing_plus) {
    if (strcmp(format,"f-64") == 0){ //use c formatting.
        char* float_str = (char*)malloc(50 * sizeof(char));
        sprintf(float_str,"%23.17e",val);
        *result = append_to_string(*result,float_str);
        free(float_str);
        return;
    } else if(strcmp(format,"f-32") == 0){ //use c formatting.
        char* float_str = (char*)malloc(40 * sizeof(char));
        sprintf(float_str,"%13.8e",val);
        *result = append_to_string(*result,float_str);
        free(float_str);
        return;
    }
    int width = 0, decimal_digits = 0;
    long integer_part = (long)fabs(val);
    double decimal_part = fabs(val) - integer_part;

    int sign_width = (val < 0) ? 1 : 0; // Negative sign
    bool sign_plus_exist = (use_sing_plus && val>=0); // Positive sign
    int integer_length = (integer_part == 0) ? 1 : (int)log10(integer_part) + 1;

    // parsing the format
    char* dot_pos = strchr(format, '.');
    if (dot_pos != NULL) {
        decimal_digits = atoi(dot_pos + 1);
        width = atoi(format + 1);
    }

    double rounding_factor = pow(10, -decimal_digits);
    decimal_part = round(decimal_part / rounding_factor) * rounding_factor;

    if (decimal_part >= 1.0) {
        integer_part += 1;
        decimal_part -= 1.0;
    }

    char int_str[64];
    sprintf(int_str, "%ld", integer_part);

    // TODO: This will work for up to `F65.60` but will fail for:
    // print "(F67.62)", 1.23456789101112e-62_8
    char dec_str[64];
    sprintf(dec_str, "%.*f", decimal_digits, decimal_part);
    // removing the leading "0." from the formatted decimal part
    memmove(dec_str, dec_str + 2, strlen(dec_str));

    // Determine total length needed
    int total_length =  sign_width      + 
                        integer_length  +
                        1 /*dot `.`*/   +
                        decimal_digits  +
                        sign_plus_exist ;
    
    if (width == 0) {
        width = total_length;
    }

    char formatted_value[128] = "";

    int spaces = width - total_length;
    for (int i = 0; i < spaces; i++) {
        strcat(formatted_value, " ");
    }
    if(sign_plus_exist){
        strcat(formatted_value, "+");
    }
    if (val < 0) {
        strcat(formatted_value, "-");
    }
    if (integer_part == 0 && format[1] == '0') {
        strcat(formatted_value, "");
    } else {
        strcat(formatted_value, int_str);
    }
    strcat(formatted_value, ".");
    strcat(formatted_value, dec_str);

    // checking for overflow
    if (strlen(formatted_value) > width) {
        for (int i = 0; i < width; i++) {
            *result = append_to_string(*result, "*");
        }
    } else {
        *result = append_to_string(*result, formatted_value);
    }
}

/*
`handle_en` - Formats a floating-point number using a Fortran-style "EN" format.

NOTE: The function allocates memory for the formatted result, which is returned via
the `result` parameter. It is the responsibility of the caller to free this memory
using `free(*result)` after it is no longer needed.
*/
void handle_en(char* format, double val, int scale, char** result, char* c, bool is_signed_plus) {
    int width, decimal_digits;
    char *num_pos = format, *dot_pos = strchr(format, '.');
    decimal_digits = atoi(++dot_pos);
    while (!isdigit(*num_pos)) num_pos++;
    width = atoi(num_pos);
    bool sign_plus_exist = (is_signed_plus && val >= 0); // `SP` specifier
    // Calculate exponent
    int exponent = 0;
    if (val != 0.0) {
        exponent = (int)floor(log10(fabs(val)));
        int remainder = exponent % 3;
        if (remainder < 0) remainder += 3;
        exponent -= remainder;
    }

    double scaled_val = val / pow(10, exponent);

    // Prepare value string
    char val_str[128];
    sprintf(val_str, "%.*lf", decimal_digits, scaled_val);

    // Truncate unnecessary zeros
    char* ptr = strchr(val_str, '.');
    if (ptr) {
        char* end_ptr = ptr;
        while (*end_ptr != '\0') end_ptr++;
        end_ptr--;
        while (*end_ptr == '0' && end_ptr > ptr) end_ptr--;
        *(end_ptr + 1) = '\0';
    }

    // Allocate a larger buffer
    char formatted_value[256];  // Increased size to accommodate larger exponent values
    int n = snprintf(formatted_value, sizeof(formatted_value), "%s%s%+03d", val_str, c, exponent);
    if (n >= sizeof(formatted_value)) {
        fprintf(stderr, "Error: output was truncated. Needed %d characters.\n", n);
    }

    // Handle width and padding
    char* final_result = malloc(width + 1);
    int padding = width - strlen(formatted_value) - sign_plus_exist;
    if (padding > 0) {
        memset(final_result, ' ', padding);
        if(sign_plus_exist){final_result[padding] = '+';}
        strcpy(final_result + padding + sign_plus_exist, formatted_value);
    } else {
        if(sign_plus_exist){final_result[0] = '+';}
        strncpy(final_result + is_signed_plus /*Move on char*/, formatted_value, width);
        final_result[width] = '\0';
    }

    // Assign the result to the output parameter
    *result = append_to_string(*result, final_result);
}

void parse_deciml_format(char* format, int* width_digits, int* decimal_digits, int* exp_digits) {
    *width_digits = -1;
    *decimal_digits = -1;
    *exp_digits = -1;

    char *width_digits_pos = format;
    while (!isdigit(*width_digits_pos)) {
        width_digits_pos++;
    }
    *width_digits = atoi(width_digits_pos);

    // dot_pos exists, we previous checked for it in `parse_fortran_format`
    char *dot_pos = strchr(format, '.');
    *decimal_digits = atoi(++dot_pos);

    char *exp_pos = strchr(dot_pos, 'e');
    if(exp_pos != NULL) {
        *exp_digits = atoi(++exp_pos);
    }
}


void handle_decimal(char* format, double val, int scale, char** result, char* c, bool is_signed_plus) {
    // Consider an example: write(*, "(es10.2)") 1.123e+10
    // format = "es10.2", val = 11230000128.00, scale = 0, c = "E"

    int width_digits, decimal_digits, exp_digits;
    parse_deciml_format(format, &width_digits, &decimal_digits, &exp_digits);

    int width = width_digits;
    int sign_width = (val < 0) ? 1 : 0;
    bool sign_plus_exist = (is_signed_plus && val>=0); // Positive sign
    // sign_width = 0
    double integer_part = trunc(val);
    int integer_length = (integer_part == 0) ? 1 : (int)log10(fabs(integer_part)) + 1;
    // integer_part = 11230000128, integer_length = 11
    // width = 10, decimal_digits = 2

    #define MAX_SIZE 128
    char val_str[MAX_SIZE] = "";
    int avail_len_decimal_digits = MAX_SIZE - integer_length - sign_width - 2 /* 0.*/;
    // TODO: This will work for up to `E65.60` but will fail for:
    // print "(E67.62)", 1.23456789101112e-62_8
    sprintf(val_str, "%.*lf", avail_len_decimal_digits, val);
    // val_str = "11230000128.00..."

    int i = strlen(val_str) - 1;
    while (val_str[i] == '0') {
        val_str[i] = '\0';
        i--;
    }
    // val_str = "11230000128."

    int exp = 2;
    if (exp_digits != -1) {
        exp = exp_digits;
    }
    // exp = 2;

    char* ptr = strchr(val_str, '.');
    if (ptr != NULL) {
        memmove(ptr, ptr + 1, strlen(ptr));
    }
    // val_str = "11230000128"

    if (val < 0) {
        // removes `-` (negative) sign
        memmove(val_str, val_str + 1, strlen(val_str));
    }

    int decimal = 1;
    while (val_str[0] == '0') {
        // Used for the case: 1.123e-10
        memmove(val_str, val_str + 1, strlen(val_str));
        decimal--;
        // loop end: decimal = -9
    }
    if (tolower(format[1]) == 's') {
        scale = 1;
    }

    char exponent[12];
    if (width_digits == 0) {
        sprintf(exponent, "%+02d", (integer_length > 0 && integer_part != 0 ? integer_length - scale : decimal - scale));
    } else {
        if (val != 0) {
            sprintf(exponent, "%+0*d", exp+1, (integer_length > 0 && integer_part != 0 ? integer_length - scale : decimal - scale));
        } else {
            sprintf(exponent, "%+0*d", exp+1, (integer_length > 0 && integer_part != 0 ? integer_length - scale : decimal));
        }
        // exponent = "+10"
    }

    int FIXED_CHARS_LENGTH = 1 + 1 + 1; // digit, ., E
    int exp_length = strlen(exponent);

    if (width == 0) {
        if (decimal_digits == 0) {
            decimal_digits = 9;
        }
        width = sign_width + decimal_digits + FIXED_CHARS_LENGTH + exp_length;
    }
    if (decimal_digits > width - FIXED_CHARS_LENGTH) {
        perror("Specified width is not enough for the specified number of decimal digits.\n");
    }
    int zeroes_needed = decimal_digits - (strlen(val_str) - integer_length);
    for(int i=0; i < zeroes_needed; i++) {
        strcat(val_str, "0");
    }

    char formatted_value[64] = "";
    int spaces = width - (sign_width + decimal_digits + FIXED_CHARS_LENGTH + exp_length + sign_plus_exist);
    // spaces = 2
    for (int i = 0; i < spaces; i++) {
        strcat(formatted_value, " ");
    }

    if (scale > 1) {
        decimal_digits -= scale - 1;
    }

    if (sign_width == 1) {
        // adds `-` (negative) sign
        strcat(formatted_value, "-");
    } else if(sign_plus_exist){ // `SP specifier`
        strcat(formatted_value, "+");
    }

    if (scale <= 0) {
        strcat(formatted_value, "0.");
        for (int k = 0; k < abs(scale); k++) {
            strcat(formatted_value, "0");
        }
        int zeros = 0;
        while(val_str[zeros] == '0') zeros++;
        // TODO: figure out a way to round decimals with value < 1e-15
        if (decimal_digits + scale < strlen(val_str) && val != 0 && decimal_digits + scale - zeros<= 15) {
            val_str[15] = '\0';
            long long t = (long long)round((long double)atoll(val_str) / (long long)pow(10, (strlen(val_str) - decimal_digits - scale)));
            sprintf(val_str, "%lld", t);
            int index = zeros;
            while(index--) strcat(formatted_value, "0");
        }
        strncat(formatted_value, val_str, decimal_digits + scale - zeros);
    } else {
        char* temp = substring(val_str, 0, scale);
        strcat(formatted_value, temp);
        strcat(formatted_value, ".");
        // formatted_value = "  1."
        char* new_str = substring(val_str, scale, strlen(val_str));
        // new_str = "1230000128" case:  1.123e+10
        int zeros = 0;
        if (decimal_digits < strlen(new_str) && decimal_digits + scale <= 15) {
            new_str[15] = '\0';
            zeros = strspn(new_str, "0");
            long long t = (long long)round((long double)atoll(new_str) / (long long) pow(10, (strlen(new_str) - decimal_digits)));
            sprintf(new_str, "%lld", t);
            // new_str = 12
            int index = zeros;
            while(index--) {
                memmove(new_str + 1, new_str, strlen(new_str)+1);
                new_str[0] = '0';
            }
        }
        new_str[decimal_digits] = '\0';
        strcat(formatted_value, new_str);
        // formatted_value = "  1.12"
        free(new_str);
        free(temp);
    }

    strcat(formatted_value, c);
    // formatted_value = "  1.12E"



    strcat(formatted_value, exponent);
    // formatted_value = "  1.12E+10"

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
        // result = "  1.12E+10"
    }
}
void handle_SP_specifier(char** result, bool is_positive_value){
    char positive_sign_string[] = "+";
    if(is_positive_value) append_to_string(*result, positive_sign_string);
}
/*
Ignore blank space characters within format specification, except
within character string edit descriptor

E.g.; "('Number : ', I 2, 5 X, A)" becomes '('Number : ', I2, 5X, A)'
*/
char* remove_spaces_except_quotes(const char* format) {
    int len = strlen(format);
    char* cleaned_format = malloc(len + 1);

    int i = 0, j = 0;
    // don't remove blank spaces from within character
    // string editor descriptor
    bool in_quotes = false;
    char current_quote = '\0';

    while (format[i] != '\0') {
        char c = format[i];
        if (c == '"' || c == '\'') {
            if (i == 0 || format[i - 1] != '\\') {
                // toggle in_quotes and set current_quote on entering or exiting quotes
                if (!in_quotes) {
                    in_quotes = true;
                    current_quote = c;
                } else if (current_quote == c) {
                    in_quotes = false;
                }
            }
        }

        if (!isspace(c) || in_quotes) {
            cleaned_format[j++] = c; // copy non-space characters or any character within quotes
        }

        i++;
    }

    cleaned_format[j] = '\0';
    return cleaned_format;
}

/**
 * parse fortran format string by extracting individual 'format specifiers'
 * (e.g. 'i', 't', '*' etc.) into an array of strings
 *
 * `char* format`: the string we need to split into format specifiers
 * `int* count`  : store count of format specifiers (passed by reference from caller)
 * `item_start`  :
 *
 * e.g. "(I5, F5.2, T10)" is split separately into "I5", "F5.2", "T10" as
 * format specifiers
*/
char** parse_fortran_format(char* format, int64_t *count, int64_t *item_start) {
    char** format_values_2 = (char**)malloc((*count + 1) * sizeof(char*));
    int format_values_count = *count;
    int index = 0 , start = 0;
    while (format[index] != '\0') {
        char** ptr = (char**)realloc(format_values_2, (format_values_count + 1) * sizeof(char*));
        if (ptr == NULL) {
            perror("Memory allocation failed.\n");
            free(format_values_2);
        } else {
            format_values_2 = ptr;
        }
        switch (tolower(format[index])) {
            case ',' :
                break;
            case '/' :
            case ':' :
                format_values_2[format_values_count++] = substring(format, index, index+1);
                break;
            case '*' :
                format_values_2[format_values_count++] = substring(format, index, index+1);
                break;
            case '"' :
                start = index++;
                while (format[index] != '"') {
                    index++;
                }
                format_values_2[format_values_count++] = substring(format, start, index+1);

                break;
            case '\'' :
                start = index++;
                while (format[index] != '\'') {
                    index++;
                }
                format_values_2[format_values_count++] = substring(format, start, index+1);
                break;
            case 'a' :
                start = index++;
                while (isdigit(format[index])) {
                    index++;
                }
                format_values_2[format_values_count++] = substring(format, start, index);
                index--;
                break;
            case 'e' :
                start = index++;
                bool edot = false;
                bool is_en_formatting = false;
                if (tolower(format[index]) == 'n') {
                    index++;  // move past the 'N'
                    is_en_formatting = true;
                }
                if (tolower(format[index]) == 's') index++;
                while (isdigit(format[index])) index++;
                if (format[index] == '.') {
                    edot = true;
                    index++;
                } else {
                    printf("Error: Period required in format specifier\n");
                    exit(1);
                }
                while (isdigit(format[index])) index++;
                if (edot && (tolower(format[index]) == 'e' || tolower(format[index]) == 'n')) {
                    index++;
                    while (isdigit(format[index])) index++;
                }
                format_values_2[format_values_count++] = substring(format, start, index);
                index--;
                break;
            case 'i' :
            case 'd' :
            case 'f' :
            case 'l' :
                start = index++;
                bool dot = false;
                if(tolower(format[index]) == 's') index++;
                while (isdigit(format[index])) index++;
                if (format[index] == '.') {
                    dot = true;
                    index++;
                }
                while (isdigit(format[index])) index++;
                if (dot && tolower(format[index]) == 'e') {
                    index++;
                    while (isdigit(format[index])) index++;
                }
                format_values_2[format_values_count++] = substring(format, start, index);
                index--;
                break;
            case 's': 
                start = index++;
                if( format[index] == ','          || // 'S'  (default sign)
                    tolower(format[index]) == 'p' || // 'SP' (sign plus)
                    tolower(format[index]) == 's'    // 'SS' (sign suppress)
                    ){
                    if(format[index] == ',') --index; // Don't consume
                    format_values_2[format_values_count++] = substring(format, start, index+1);
                } else {
                    fprintf(stderr, "Error: Invalid format specifier. After 's' specifier\n");
                    exit(1);
                }
                break;
            case '(' :
                start = index++;
                while (format[index] != ')') index++;
                format_values_2[format_values_count++] = substring(format, start, index+1);
                *item_start = format_values_count;
                break;
            case 't' :
                // handle 'T', 'TL' & 'TR' editing see section 13.8.1.2 in 24-007.pdf
                start = index++;
                if (tolower(format[index]) == 'l' || tolower(format[index]) == 'r') {
                     index++;  // move past 'L' or 'R'
                }
                // raise error when "T/TL/TR" is specified itself or with
                // non-positive width
                if (!isdigit(format[index])) {
                    // TODO: if just 'T' is specified the error message will print 'T,', fix it
                    printf("Error: Positive width required with '%c%c' descriptor in format string\n",
                        format[start], format[start + 1]);
                    exit(1);
                }
                while (isdigit(format[index])) {
                    index++;
                }
                format_values_2[format_values_count++] = substring(format, start, index);
                index--;
                break;
            default :
                if (
                    (format[index] == '-' && isdigit(format[index + 1]) && tolower(format[index + 2]) == 'p')
                    || ((isdigit(format[index])) && tolower(format[index + 1]) == 'p')) {
                    start = index;
                    index = index + 1 + (format[index] == '-');
                    format_values_2[format_values_count++] = substring(format, start, index + 1);
                } else if (isdigit(format[index])) {
                    start = index;
                    while (isdigit(format[index])) index++;
                    char* repeat_str = substring(format, start, index);
                    int repeat = atoi(repeat_str);
                    free(repeat_str);
                    format_values_2 = (char**)realloc(format_values_2, (format_values_count + repeat + 1) * sizeof(char*));
                    if (format[index] == '(') {
                        start = index++;
                        while (format[index] != ')') index++;
                        *item_start = format_values_count+1;
                        for (int i = 0; i < repeat; i++) {
                            format_values_2[format_values_count++] = substring(format, start, index+1);
                        }
                    } else {
                        start = index++;
                        if (isdigit(format[index])) {
                            while (isdigit(format[index])) index++;
                            if (format[index] == '.') index++;
                            while (isdigit(format[index])) index++;
                        }
                        for (int i = 0; i < repeat; i++) {
                            format_values_2[format_values_count++] = substring(format, start, index);
                        }
                        index--;
                    }
                } else if (format[index] != ' ') {
                    fprintf(stderr, "Unsupported or unrecognized `%c` in format string\n", format[index]);
                    exit(1);
                }
        }
        index++;
    }
    *count = format_values_count;
    return format_values_2;
}

#define primitive_types_cnt  10
typedef enum primitive_types{
    INTEGER_64_TYPE = 0,
    INTEGER_32_TYPE = 1,
    INTEGER_16_TYPE = 2,
    INTEGER_8_TYPE = 3,
    FLOAT_64_TYPE = 4,
    FLOAT_32_TYPE = 5,
    CHARACTER_TYPE = 6,
    LOGICAL_TYPE = 7,
    CPTR_VOID_PTR_TYPE = 8,
    NONE_TYPE = 9
} Primitive_Types;

const int primitive_type_sizes[primitive_types_cnt] = 
                                        {sizeof(int64_t), sizeof(int32_t), sizeof(int16_t),
                                         sizeof(int8_t) , sizeof(double), sizeof(float), 
                                         sizeof(char*), sizeof(bool), sizeof(void*), 0 /*Important to be zero*/};

char primitive_enum_to_format_specifier(Primitive_Types primitive_enum){
    switch(primitive_enum){
        case INTEGER_8_TYPE:
        case INTEGER_16_TYPE:
        case INTEGER_32_TYPE:
        case INTEGER_64_TYPE:
            return 'i';
            break;
        case FLOAT_32_TYPE:
        case FLOAT_64_TYPE:
            return 'f';
            break;
        case CHARACTER_TYPE:
            return 'a';
            break;
        case LOGICAL_TYPE:
            return 'l';
            break;
        case CPTR_VOID_PTR_TYPE:
            return 'P'; // Not actual fortran specifier.
            break;
        default:
            fprintf(stderr,"Compiler Error : Unidentified Type %d\n", primitive_enum);
            exit(0);
    }

}
bool is_format_match(char format_value, Primitive_Types current_arg_type){
    char current_arg_correct_format = 
        primitive_enum_to_format_specifier(current_arg_type); 

    char lowered_format_value = tolower(format_value);
    if(lowered_format_value == 'd' || lowered_format_value == 'e'){
        lowered_format_value = 'f';
    }
    // Special conditions that are allowed by gfortran.
    bool special_conditions = (lowered_format_value == 'l' && current_arg_correct_format == 'a') ||
                              (lowered_format_value == 'a' && current_arg_correct_format == 'l');
    if(lowered_format_value != current_arg_correct_format && !special_conditions){
        return false;
    } else {
        return true;
    }
}


typedef struct stack {
    int64_t* p;
    int32_t stack_size;
    int32_t top_index;
} Stack;  

Stack* create_stack(){
    Stack* s = (Stack*)malloc(sizeof(Stack));
    s->stack_size = 10; // intial value
    s->p = (int64_t*)malloc(s->stack_size * sizeof(int64_t));
    s->top_index = -1;
    return s;
}

void push_stack(Stack* x, int64_t val){
    if(x->top_index == x->stack_size - 1){ // Check if extending is needed.
        x->stack_size *= 2;
        x->p = (int64_t*)realloc(x->p, x->stack_size * sizeof(int64_t));
    }
    x->p[++x->top_index] = val;
}

void pop_stack(Stack* x){
    if(x->top_index == -1){
        fprintf(stderr,"Compiler Internal Error.\n");
        exit(1);
    }
    --x->top_index;
}

static inline int64_t get_stack_top(struct stack* s){
    return s->p[s->top_index];
}

static inline bool stack_empty(Stack* s){
    return s->top_index == -1;
}
void free_stack(Stack* x){
    free(x->p);
    free(x);
}



typedef struct serialization_info{
    const char* serialization_string;
    int32_t current_stop; // current stop index in the serialization_string.
    Stack* array_sizes_stack; // Holds the sizes of the arrays (while nesting).
    Stack* array_serialiation_start_index; // Holds the index of '[' char in serialization
    Primitive_Types current_element_type;
    struct current_arg_info{
        va_list* args;
        void* current_arg; // holds pointer to the arg being printed (Scalar or Vector) 
        bool is_complex;
    } current_arg_info;
    struct array_sizes{ // Sizes of passed arrays
        int64_t* ptr;
        int32_t current_index;
    } array_sizes;
    bool just_peeked; // Flag to indicate if we just peeked the next element.
} Serialization_Info;



// Transforms serialized array size from string to int64 (characters into numbers).
int64_t get_serialized_array_size(struct serialization_info* s_info){
    int64_t array_size = 0;
    while(isdigit(s_info->serialization_string[s_info->current_stop])){
        array_size = array_size * 10 + (s_info->serialization_string[s_info->current_stop++] - '0');
    }
    return array_size;
}

/* Sets primitive type for the current argument
 by parsing through the serialization string. */
void set_current_PrimitiveType(Serialization_Info* s_info){
    Primitive_Types *PrimitiveType = &s_info->current_element_type;
    switch (s_info->serialization_string[s_info->current_stop++])
    {
    case 'I':
        switch (s_info->serialization_string[s_info->current_stop++])
        {
        case '8':
            *PrimitiveType = INTEGER_64_TYPE;
            break;
        case '4':
            *PrimitiveType = INTEGER_32_TYPE;
            break;
        case '2':
            *PrimitiveType = INTEGER_16_TYPE;
            break;
        case '1':
            *PrimitiveType = INTEGER_8_TYPE;
            break;
        default:
            fprintf(stderr, "RunTime - compiler internal error"
                " : Unidentified Print Types Serialization --> %s\n",
                    s_info->serialization_string);
            exit(1);
            break;
        }
        break;
    case 'R':
        switch (s_info->serialization_string[s_info->current_stop++])
        {
        case '8':
            *PrimitiveType = FLOAT_64_TYPE;
            break;
        case '4':
            *PrimitiveType = FLOAT_32_TYPE;
            break;
        default:
            fprintf(stderr, "RunTime - compiler" 
            "internal error : Unidentified Print Types Serialization --> %s\n",
                    s_info->serialization_string);
            exit(1);
            break;
        }
        break;
    case 'L':
        *PrimitiveType = LOGICAL_TYPE;
        break;
    case 'S':
        *PrimitiveType = CHARACTER_TYPE;
        break;
    case 'C':
        ASSERT_MSG(s_info->serialization_string[s_info->current_stop++] == 'P' &&
            s_info->serialization_string[s_info->current_stop++] == 't' &&
            s_info->serialization_string[s_info->current_stop++] == 'r',
            "%s\n",s_info->serialization_string);
        *PrimitiveType = CPTR_VOID_PTR_TYPE;
        break;
    default:
        fprintf(stderr, "RunTime - compiler internal error"
            " : Unidentified Print Types Serialization --> %s\n",
                s_info->serialization_string);
        exit(1);
        break;
    }
}

/*
    Fetches the type of next element that will get printed + 
    sets the `current_arg` to the desired pointer.
    Return `false` if no elements remaining.
*/
bool move_to_next_element(struct serialization_info* s_info, bool peek){
    // Handle `peek` flag logic
    if(s_info->just_peeked && peek ||
        (s_info->just_peeked && !peek)){ // already `s_info` is set with current element.
        s_info->just_peeked = peek;
        return s_info->current_element_type != NONE_TYPE;
    } else{ // Get next.
        s_info->just_peeked = peek;
    }

    while(true){
        char cur = s_info->serialization_string[s_info->current_stop];
        // Zero-size array OR inside zero-size array --> e.g. (0[(I8,4[S])])
        bool zero_size = (!stack_empty(s_info->array_sizes_stack) &&
                            (get_stack_top(s_info->array_sizes_stack) == 0)); 
        if(isdigit(cur)){ // ArraySize --> `50[I4]`
            int64_t array_size = get_serialized_array_size(s_info);
            ASSERT_MSG(s_info->serialization_string[s_info->current_stop] == '[',
                "RunTime - Compiler Internal error "
                ": Wrong serialization for print statement --> %s\n",
                s_info->serialization_string);
            zero_size ? push_stack(s_info->array_sizes_stack, 0):
                        push_stack(s_info->array_sizes_stack, array_size);
        } else if (cur == '['){ //ArrayStart
            if(stack_empty(s_info->array_sizes_stack)){// Runtime-NonStringSerialized arraysize.
                push_stack(s_info->array_sizes_stack,
                    s_info->array_sizes.ptr[s_info->array_sizes.current_index++]); 
            }
            // Remeber where the array starts to handle this nested case --> `10[(10[I4]))]`
            ++s_info->current_stop;
            push_stack(s_info->array_serialiation_start_index, s_info->current_stop);
        } else if (cur == ']'){ // ArrayEnd
            if(!zero_size &&  (get_stack_top(s_info->array_sizes_stack) - 1 == 0) || 
                (zero_size &&  (get_stack_top(s_info->array_sizes_stack) - 1 == -1))){ // Move to the next element.
                pop_stack(s_info->array_sizes_stack);
                pop_stack(s_info->array_serialiation_start_index);
                ++s_info->current_stop;
            } else { // Decrement Size + Move to the begining of the array.
                ASSERT_MSG(!zero_size,
                    "%s\n", "Zero-size vector shouldn't go back to the begining.");
                int64_t arr_size_decremented = get_stack_top(s_info->array_sizes_stack) - 1;
                pop_stack(s_info->array_sizes_stack);
                push_stack(s_info->array_sizes_stack, arr_size_decremented);
                s_info->current_stop = get_stack_top(s_info->array_serialiation_start_index);
            }
        } else if (cur == '(' || cur == '{') { // StructStart, complexStart
            s_info->current_arg_info.is_complex = (cur == '{');
            push_stack(s_info->array_sizes_stack, -1000000); // dummy size. needed to know when to move from arg* to another.
            ++s_info->current_stop;
        } else if (cur == ')' || cur == '}') { // StructEnd, ComplexEnd
            s_info->current_arg_info.is_complex = false;
            pop_stack(s_info->array_sizes_stack);
            ++s_info->current_stop;
        } else if (cur == ','){ // Separator between scalars or in compound type --> `I4,R8`, (I4,R8)`.
            ++s_info->current_stop;
            // Only move from passed arg to another in the `va_list` when we don't have struct or array in process.
            if(stack_empty(s_info->array_sizes_stack)){ 
                ASSERT(stack_empty(s_info->array_serialiation_start_index));
                s_info->current_arg_info.current_arg = va_arg(*s_info->current_arg_info.args, void*);
                s_info->current_element_type = NONE_TYPE; // Important to set type to none when moving from whole argument to another
            } 
        } else if(cur == '\0'){ // End of Serialization.
            ASSERT( stack_empty(s_info->array_sizes_stack) && 
                    stack_empty(s_info->array_serialiation_start_index));
            s_info->current_arg_info.current_arg = NULL;
            s_info->current_element_type = NONE_TYPE;
            return false;
        } else { // Type
            if(zero_size) {
                set_current_PrimitiveType(s_info);/*Keep deserializing*/
                continue;
            }
            // move the current_arg by the size of current_arg.
            s_info->current_arg_info.current_arg = (void*)
                            ((char*)s_info->current_arg_info.current_arg +
                                primitive_type_sizes[s_info->current_element_type]); // char* cast needed for windows MinGW.
            set_current_PrimitiveType(s_info);
            return true;
        }
    }
}



void print_into_string(Serialization_Info* s_info,  char* result){
    void* arg = s_info->current_arg_info.current_arg;
    switch (s_info->current_element_type){
        case INTEGER_64_TYPE:
            sprintf(result, "%"PRId64, *(int64_t*)arg);
            break;
        case INTEGER_32_TYPE:
            sprintf(result, "%d", *(int32_t*)arg);
            break;
        case INTEGER_16_TYPE:
            sprintf(result, "%hi", *(int16_t*)arg);
            break;
        case INTEGER_8_TYPE:
            sprintf(result, "%hhi", *(int8_t*)arg);
            break;
        case FLOAT_64_TYPE:
            if(s_info->current_arg_info.is_complex){
                double real = *(double*)arg;
                move_to_next_element(s_info, false);
                double imag = *(double*)s_info->current_arg_info.current_arg;
                sprintf(result, "(%23.17e, %23.17e)", real, imag);
            } else {
                sprintf(result, "%23.17e", *(double*)arg);
            }
            break;
        case FLOAT_32_TYPE:
            if(s_info->current_arg_info.is_complex){
                float real = *(float*)arg;
                move_to_next_element(s_info, false);
                double imag = *(float*)s_info->current_arg_info.current_arg;
                sprintf(result, "(%13.8e, %13.8e)", real, imag);
            } else {
                sprintf(result, "%13.8e", *(float*)arg);
            }
            break;
        case LOGICAL_TYPE:
            sprintf(result, "%c", (*(bool*)arg)? 'T' : 'F');
            break;
        case CHARACTER_TYPE:
            if(*(char**)arg == NULL){
                sprintf(result, "%s", " ");
            } else {
                sprintf(result, "%s",*(char**)arg);
            }
            break;
        case CPTR_VOID_PTR_TYPE:
            sprintf(result, "%p",*(void**)arg);
            break;
        default :
            fprintf(stderr, "Unknown type");
            exit(1);
    }

}

void default_formatting(char** result, struct serialization_info* s_info){
    int64_t result_capacity = 100;
    int64_t result_size = 0;
    const int default_spacing_len = 4;
    const char* default_spacing = "    ";
    ASSERT(default_spacing_len == strlen(default_spacing));
    *result = realloc(*result, result_capacity + 1 /*Null Character*/ );

    while(move_to_next_element(s_info, false)){
        int size_to_allocate;
        if(s_info->current_element_type == CHARACTER_TYPE && 
            *(char**)s_info->current_arg_info.current_arg != NULL){
            size_to_allocate = (strlen(*(char**)s_info->current_arg_info.current_arg)
                                 + default_spacing_len + 1) * sizeof(char);
        } else {
            size_to_allocate = (60 + default_spacing_len) * sizeof(char);
        }
        int64_t old_capacity = result_capacity;
        while(result_capacity <= size_to_allocate + result_size){ // Check if string extension is needed.
            if(result_size + size_to_allocate > result_capacity*2){
                result_capacity = size_to_allocate + result_size ;
            } else {
                result_capacity *=2;
            }
        }
        if(result_capacity != old_capacity){*result = (char*)realloc(*result, result_capacity + 1);}
        if(result_size > 0){
            strcpy((*result)+result_size, default_spacing);
            result_size+=default_spacing_len;
        }
        print_into_string(s_info,  (*result) + result_size);
        int64_t printed_arg_size = strlen((*result) + result_size);
        result_size += printed_arg_size;
    }
}
void free_serialization_info(Serialization_Info* s_info){
    free(s_info->array_sizes.ptr);
    free_stack(s_info->array_sizes_stack);
    free_stack(s_info->array_serialiation_start_index);
    va_end(*s_info->current_arg_info.args);
}

LFORTRAN_API char* _lcompilers_string_format_fortran(const char* format, const char* serialization_string, 
    int32_t array_sizes_cnt, ...)
{
    va_list args;
    va_start(args, array_sizes_cnt);
    char* result = (char*)malloc(sizeof(char)); //TODO : the consumer of this string needs to free it.
    result[0] = '\0';

    // Setup s_info
    struct serialization_info s_info;
    s_info.serialization_string = serialization_string;
    s_info.array_serialiation_start_index = create_stack();
    s_info.array_sizes_stack = create_stack();
    s_info.current_stop = 0;
    s_info.current_arg_info.args = &args;
    s_info.current_element_type = NONE_TYPE;
    s_info.current_arg_info.is_complex = false;
    s_info.array_sizes.current_index = 0;
    s_info.just_peeked = false;
    int64_t* array_sizes = (int64_t*) malloc(array_sizes_cnt * sizeof(int64_t));
    for(int i=0; i<array_sizes_cnt; i++){
        array_sizes[i] = va_arg(args, int64_t);
    }
    s_info.array_sizes.ptr = array_sizes;
    s_info.current_arg_info.current_arg = va_arg(args, void*);

    if(!s_info.current_arg_info.current_arg && 
        s_info.serialization_string[s_info.current_stop] !='\0')
    {fprintf(stderr,"Internal Error : default formatting error\n");exit(1);}


    if(format == NULL){
        default_formatting(&result, &s_info);
        free_serialization_info(&s_info);
        return result;
    }

    int64_t format_values_count = 0,item_start_idx=0;
    char** format_values;
    char* modified_input_string;
    char* cleaned_format = remove_spaces_except_quotes(format);
    if (!cleaned_format) {
        free_serialization_info(&s_info);
        return NULL;
    }
    int len = strlen(cleaned_format);
    modified_input_string = (char*)malloc((len+1) * sizeof(char));
    strncpy(modified_input_string, cleaned_format, len);
    modified_input_string[len] = '\0';
    if (cleaned_format[0] == '(' && cleaned_format[len-1] == ')') {
        memmove(modified_input_string, modified_input_string + 1, strlen(modified_input_string));
        modified_input_string[len-2] = '\0';
    }
    format_values = parse_fortran_format(modified_input_string,&format_values_count,&item_start_idx);
    /*
    is_SP_specifier = false  --> 'S' OR 'SS'
    is_SP_specifier = true  --> 'SP'
    */
    bool is_SP_specifier = false;
    int item_start = 0;
    bool array = false;
    bool BreakWhileLoop= false;
    while (1) {
        int scale = 0;
        bool is_array = false;
        bool array_looping = false;
        for (int i = item_start; i < format_values_count; i++) {
            char* value;
            if(format_values[i] == NULL) continue;
            value = format_values[i];
            if (value[0] == '(' && value[strlen(value)-1] == ')') {
                value[strlen(value)-1] = '\0';
                int64_t new_fmt_val_count = 0;
                char** new_fmt_val = parse_fortran_format(++value,&new_fmt_val_count,&item_start_idx);

                char** ptr = (char**)realloc(format_values, (format_values_count + new_fmt_val_count + 1) * sizeof(char*));
                if (ptr == NULL) {
                    perror("Memory allocation failed.\n");
                    free(format_values);
                } else {
                    format_values = ptr;
                }
                for (int k = format_values_count - 1; k >= i+1; k--) {
                    format_values[k + new_fmt_val_count] = format_values[k];
                }
                for (int k = 0; k < new_fmt_val_count; k++) {
                    format_values[i + 1 + k] = new_fmt_val[k];
                }
                format_values_count = format_values_count + new_fmt_val_count;
                free(format_values[i]);
                format_values[i] = NULL;
                free(new_fmt_val);
                continue;
            }

            if (value[0] == ':') {
                if (!move_to_next_element(&s_info, true)) break;
                continue;
            } else if (value[0] == '/') {
                result = append_to_string(result, "\n");
            } else if (value[0] == '*') {
                array = true;
            } else if (isdigit(value[0]) && tolower(value[1]) == 'p') {
                // Scale Factor nP
                scale = atoi(&value[0]);
            } else if (value[0] == '-' && isdigit(value[1]) && tolower(value[2]) == 'p') {
                char temp[3] = {value[0],value[1],'\0'};
                scale = atoi(temp);
            } else if ((value[0] == '\"' && value[strlen(value) - 1] == '\"') ||
                (value[0] == '\'' && value[strlen(value) - 1] == '\'')) {
                // String
                value = substring(value, 1, strlen(value) - 1);
                result = append_to_string(result, value);
                free(value);
            } else if (tolower(value[strlen(value) - 1]) == 'x') {
                result = append_to_string(result, " ");
            } else if (tolower(value[0]) == 's') {
                is_SP_specifier = ( strlen(value) == 2 /*case 'S' speicifer*/ &&
                                    tolower(value[1]) == 'p'); 
            } else if (tolower(value[0]) == 't') {
                if (tolower(value[1]) == 'l') {
                    // handle "TL" format specifier
                    int tab_left_pos = atoi(value + 2);
                    int current_length = strlen(result);
                    if (tab_left_pos > current_length) {
                        result[0] = '\0';
                    } else {
                        result[current_length - tab_left_pos] = '\0';
                    }
                } else if (tolower(value[1]) == 'r') {
                    // handle "TR" format specifier
                    int tab_right_pos = atoi(value + 2);
                    int current_length = strlen(result);
                    int spaces_needed = tab_right_pos;
                    if (spaces_needed > 0) {
                        char* spaces = (char*)malloc((spaces_needed + 1) * sizeof(char));
                        memset(spaces, ' ', spaces_needed);
                        spaces[spaces_needed] = '\0';
                        result = append_to_string(result, spaces);
                        free(spaces);
                    }
                } else {
                    if (!move_to_next_element(&s_info, true)) break;
                    int tab_position = atoi(value + 1);
                    int current_length = strlen(result);
                    int spaces_needed = tab_position - current_length - 1;
                    if (spaces_needed > 0) {
                        char* spaces = (char*)malloc((spaces_needed + 1) * sizeof(char));
                        memset(spaces, ' ', spaces_needed);
                        spaces[spaces_needed] = '\0';
                        result = append_to_string(result, spaces);
                        free(spaces);
                    } else if (spaces_needed < 0) {
                        // Truncate the string to the length specified by Tn
                        // if the current position exceeds it
                        if (tab_position < current_length) {
                            // Truncate the string at the position specified by Tn
                            result[tab_position] = '\0';
                        }
                    }
                }
            } else {
                if (!move_to_next_element(&s_info, false)) break;
                if (!is_format_match(
                        tolower(value[0]), s_info.current_element_type)){
                    char* type; // For better error message.
                    switch (primitive_enum_to_format_specifier(s_info.current_element_type))
                    {
                        case 'i':
                            type = "INTEGER";
                            break;
                        case 'f':
                            type = "REAL";
                            break;
                        case 'l':
                            type = "LOGICAL";
                            break;
                        case 'a':
                            type = "CHARACTER";
                            break;
                    }
                    free(result);
                    result = (char*)malloc(150 * sizeof(char));
                    sprintf(result, " Runtime Error : Got argument of type (%s), while the format specifier is (%c)\n",type ,value[0]);
                    // Special indication for error --> "\b" to be handled by `lfortran_print` or `lfortran_file_write`
                    result[0] = '\b';
                    BreakWhileLoop = true;
                    break;
                }
            
                // All formatting functions uses int64 and double.
                // We have to cast the pointers to int64 or double to avoid accessing beyond bounds.
                int64_t integer_val = 0;
                double double_val = 0;
                switch(s_info.current_element_type ){
                    case  INTEGER_64_TYPE:
                        integer_val = *(int64_t*)s_info.current_arg_info.current_arg; 
                        break;
                    case  INTEGER_32_TYPE:
                        integer_val = (int64_t)*(int32_t*)s_info.current_arg_info.current_arg; 
                        break;
                    case  INTEGER_16_TYPE:
                        integer_val = (int64_t)*(int16_t*)s_info.current_arg_info.current_arg; 
                        break;
                    case  INTEGER_8_TYPE:
                        integer_val = (int64_t)*(int8_t*)s_info.current_arg_info.current_arg; 
                        break;
                    case  FLOAT_64_TYPE:
                        double_val = *(double*)s_info.current_arg_info.current_arg; 
                        break;
                    case  FLOAT_32_TYPE:
                        double_val = (double)*(float*)s_info.current_arg_info.current_arg; 
                        break;
                    default:
                        break;
                }
                if (tolower(value[0]) == 'a') {
                    // Handle if argument is actually logical (allowed in Fortran).
                    if(s_info.current_element_type==LOGICAL_TYPE){
                        handle_logical("l",*(bool*)s_info.current_arg_info.current_arg, &result);
                        continue;
                    }
                    char* arg = *(char**)s_info.current_arg_info.current_arg;
                    if (arg == NULL) continue;
                    if (strlen(value) == 1) {
                        result = append_to_string(result, arg);
                    } else {
                        char* str = (char*)malloc((strlen(value)) * sizeof(char));
                        memmove(str, value+1, strlen(value));
                        int buffer_size = 20;
                        char* s = (char*)malloc(buffer_size * sizeof(char));
                        snprintf(s, buffer_size, "%%%s.%ss", str, str);
                        char* string = (char*)malloc((atoi(str) + 1) * sizeof(char));
                        sprintf(string,s, arg);
                        result = append_to_string(result, string);
                        free(str);
                        free(s);
                        free(string);
                    }
                } else if (tolower(value[0]) == 'i') {
                    // Integer Editing ( I[w[.m]] )
                    handle_integer(value, integer_val, &result, is_SP_specifier);
                } else if (tolower(value[0]) == 'd') {
                    // D Editing (D[w[.d]])
                    double val = *(double*)s_info.current_arg_info.current_arg;
                    handle_decimal(value, double_val, scale, &result, "D", is_SP_specifier);
                } else if (tolower(value[0]) == 'e') {
                    // Check if the next character is 'N' for EN format
                    char format_type = tolower(value[1]);
                    if (format_type == 'n') {
                        handle_en(value, double_val, scale, &result, "E", is_SP_specifier);
                    } else {
                        handle_decimal(value, double_val, scale, &result, "E", is_SP_specifier);
                    }
                } else if (tolower(value[0]) == 'f') {
                    handle_float(value, double_val, &result, is_SP_specifier);
                } else if (tolower(value[0]) == 'l') {
                    bool val = *(bool*)s_info.current_arg_info.current_arg;
                    handle_logical(value, val, &result);
                } else if (strlen(value) != 0) {
                    printf("Printing support is not available for %s format.\n",value);
                }

            }
        }
        if(BreakWhileLoop) break;
        if (move_to_next_element(&s_info, true)) {
            if (!array) {
                result = append_to_string(result, "\n");
            }
            item_start = item_start_idx;
        } else {
            break;
        }
    }
    free(modified_input_string);
    for (int i = 0;(i<format_values_count);i++) {
            free(format_values[i]);
    }
    va_end(args);
    free(format_values);
    free_serialization_info(&s_info);
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

LFORTRAN_API bool _lfortran_sis_nan(float x) {
    return isnan(x);
}

LFORTRAN_API bool _lfortran_dis_nan(double x) {
    return isnan(x);
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

// besselj0 --------------------------------------------------------------------

LFORTRAN_API double _lfortran_dbessel_j0( double x ) {
    return j0(x);
}

LFORTRAN_API float _lfortran_sbessel_j0( float x ) {
    return j0(x);
}

// besselj1 --------------------------------------------------------------------

LFORTRAN_API double _lfortran_dbessel_j1( double x ) {
    return j1(x);
}

LFORTRAN_API float _lfortran_sbessel_j1( float x ) {
    return j1(x);
}

// besseljn --------------------------------------------------------------------

LFORTRAN_API double _lfortran_dbesseljn( int n, double x ) {
    return jn(n, x);
}

LFORTRAN_API float _lfortran_sbesseljn( int n, float x ) {
    return jn(n, x);
}

// bessely0 --------------------------------------------------------------------

LFORTRAN_API double _lfortran_dbessel_y0( double x ) {
    return y0(x);
}

LFORTRAN_API float _lfortran_sbessel_y0( float x ) {
    return y0(x);
}

// bessely1 --------------------------------------------------------------------

LFORTRAN_API double _lfortran_dbessel_y1( double x ) {
    return y1(x);
}

LFORTRAN_API float _lfortran_sbessel_y1( float x ) {
    return y1(x);
}

// besselyn --------------------------------------------------------------------

LFORTRAN_API double _lfortran_dbesselyn( int n, double x ) {
    return yn(n, x);
}

LFORTRAN_API float _lfortran_sbesselyn( int n, float x ) {
    return yn(n, x);
}

uint64_t cutoff_extra_bits(uint64_t num, uint32_t bits_size, uint32_t max_bits_size) {
    if (bits_size == max_bits_size) {
        return num;
    }
    return (num & ((1lu << bits_size) - 1lu));
}

LFORTRAN_API int _lfortran_sishftc(int val, int shift_signed, int bits_size) {
    uint32_t max_bits_size = 64;
    bool negative_shift = (shift_signed < 0);
    uint32_t shift = abs(shift_signed);

    uint64_t val1 = cutoff_extra_bits((uint64_t)val, (uint32_t)bits_size, max_bits_size);
    uint64_t result;
    if (negative_shift) {
        result = (val1 >> shift) | cutoff_extra_bits(val1 << (bits_size - shift), bits_size, max_bits_size);
    } else {
        result = cutoff_extra_bits(val1 << shift, bits_size, max_bits_size) | ((val1 >> (bits_size - shift)));
    }
    return result;
}

LFORTRAN_API int64_t _lfortran_dishftc(int64_t val, int64_t shift_signed, int64_t bits_size) {
    uint32_t max_bits_size = 64;
    bool negative_shift = (shift_signed < 0);
    uint32_t shift = llabs(shift_signed);

    uint64_t val1 = cutoff_extra_bits((uint64_t)val, (uint32_t)bits_size, max_bits_size);
    uint64_t result;
    if (negative_shift) {
        result = (val1 >> shift) | cutoff_extra_bits(val1 << (bits_size - shift), bits_size, max_bits_size);
    } else {
        result = cutoff_extra_bits(val1 << shift, bits_size, max_bits_size) | ((val1 >> (bits_size - shift)));
    }
    return result;
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

LFORTRAN_API float _lfortran_ssind(float x)
{
    float radians = (x * PI) / 180.0;
    return sin(radians);
}

LFORTRAN_API double _lfortran_dsind(double x)
{
    double radians = (x * PI) / 180.0;
    return sin(radians);
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

LFORTRAN_API float _lfortran_scosd(float x)
{
    float radians = (x * PI) / 180.0;
    return cos(radians);
}

LFORTRAN_API double _lfortran_dcosd(double x)
{
    double radians = (x * PI) / 180.0;
    return cos(radians);
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

LFORTRAN_API float _lfortran_stand(float x)
{
    float radians = (x * PI) / 180.0;
    return tan(radians);
}

LFORTRAN_API double _lfortran_dtand(double x)
{
    double radians = (x * PI) / 180.0;
    return tan(radians);
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

LFORTRAN_API float _lfortran_sasind(float x)
{
    return (asin(x)*180)/PI;
}

LFORTRAN_API double _lfortran_dasind(double x)
{
    return (asin(x)*180)/PI;
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

LFORTRAN_API float _lfortran_sacosd(float x)
{
    return (acos(x)*180)/PI;
}

LFORTRAN_API double _lfortran_dacosd(double x)
{
    return (acos(x)*180)/PI;
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

LFORTRAN_API float _lfortran_satand(float x)
{
    return (atan(x)*180)/PI;
}

LFORTRAN_API double _lfortran_datand(double x)
{
    return (atan(x)*180)/PI;
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

// trunc -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_strunc(float x)
{
    return truncf(x);
}

LFORTRAN_API double _lfortran_dtrunc(double x)
{
    return trunc(x);
}

// fix -----------------------------------------------------------------------

LFORTRAN_API float _lfortran_sfix(float x)
{
    if (x > 0.0) {
        return floorf(x);
    } else {
        return ceilf(x);
    }
}

LFORTRAN_API double _lfortran_dfix(double x)
{
    if (x > 0.0) {
        return floor(x);
    } else {
        return ceil(x);
    }
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
// Allocate_allocatable-strings + Extend String ----------------------------------------------------------- 

void extend_string(char** ptr, int32_t new_size /*Null-Character Counted*/, int64_t* string_capacity){
    ASSERT_MSG(string_capacity != NULL, "%s", "string capacity is NULL");
    int64_t new_capacity;
    if((*string_capacity)*2 < new_size){
        new_capacity = new_size;
    } else {
        new_capacity = (*string_capacity)*2;
    }

    *ptr = realloc(*ptr, new_capacity);
    ASSERT_MSG(*ptr != NULL, "%s", "pointer reallocation failed!");

    *string_capacity = new_capacity;
}

LFORTRAN_API void _lfortran_allocate_string(char** ptr, int64_t desired_size /*Null-Character Counted*/
    , int64_t* string_size, int64_t* string_capacity) {
    if(*ptr == NULL && *string_size == 0 && *string_capacity == 0){
        // Start off with (inital_capacity >= 100).
        int32_t inital_capacity;
        if(100 < desired_size){
            inital_capacity = desired_size;
        } else {
            inital_capacity = 100; 
        }
        *ptr = (char*)malloc(inital_capacity);
        *string_capacity = inital_capacity;
        const int8_t null_terminated_char_len = 1;
        *string_size = desired_size - null_terminated_char_len;
    } else if(*ptr != NULL && *string_capacity != 0){
        printf("runtime error: Attempting to allocate already allocated variable\n");
        exit(1);
    } else {
        printf("Compiler Internal Error :Invalid state of string descriptor\n");
        exit(1);
    }
}

// strcpy -----------------------------------------------------------


LFORTRAN_API void _lfortran_strcpy_descriptor_string(char** x, char *y, int64_t* x_string_size, int64_t* x_string_capacity)
{
    ASSERT_MSG(x_string_size != NULL,"%s", "string size is NULL");
    ASSERT_MSG(x_string_capacity != NULL, "%s", "string capacity is NULL");
    ASSERT_MSG(((*x != NULL) && (*x_string_size <= (*x_string_capacity - 1))) ||
        (*x == NULL && *x_string_size == 0 && *x_string_capacity == 0) , "%s",
    "compiler-behavior error : string x_string_capacity < string size");
    
    if(y == NULL){
        fprintf(stderr,
        "Runtime Error : RHS allocatable-character variable must be allocated before assignment.\n");
        exit(1);
    }
    size_t y_len, x_len; 
    y_len = strlen(y); 
    x_len = y_len;

    if (*x == NULL) {
        _lfortran_allocate_string(x, y_len+1, x_string_size, x_string_capacity); // Allocate new memory for x.
    } else {
        int8_t null_char_len = 1;
        if(*x_string_capacity < (y_len + null_char_len)){
            extend_string(x, y_len+1, x_string_capacity);
        }
    }
    int64_t null_character_index = x_len;
    (*x)[null_character_index] = '\0'; 
    for (size_t i = 0; i < x_len; i++) {
        (*x)[i] = y[i];
    }
    *x_string_size = y_len;
}

LFORTRAN_API void _lfortran_strcpy_pointer_string(char** x, char *y)
{
    if(y == NULL){
        fprintf(stderr,
        "Runtime Error : RHS allocatable-character variable must be allocated before assignment.\n");
        exit(1);
    }
    size_t y_len;
    y_len = strlen(y); 
    // A workaround :
    // every LHS string that's not allocatable should have been
    // allocated a fixed-size-memory space that stays there for the whole life time of the program.
    if( *x == NULL ) {
        *x = (char*) malloc((y_len + 1) * sizeof(char));
        _lfortran_string_init(y_len + 1, *x);
    }
    for (size_t i = 0; i < strlen(*x); i++) {
        if (i < y_len) {
            x[0][i] = y[i];
        } else {
            x[0][i] = ' ';
        }
    }
}

#define MIN(x, y) ((x < y) ? x : y)

int strlen_without_trailing_space(char *str) {
    int end = strlen(str) - 1;
    while(end >= 0 && str[end] == ' ') end--;
    return end + 1;
}

int str_compare(char **s1, char **s2)
{
    int s1_len = strlen_without_trailing_space(*s1);
    int s2_len = strlen_without_trailing_space(*s2);
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
LFORTRAN_API char* _lfortran_str_item(char* s, int64_t idx) {

    int s_len = strlen(s);
    // TODO: Remove bound check in Release mode
    int64_t original_idx = idx - 1;
    if (idx < 1) idx += s_len;
    if (idx < 1 || idx >= s_len + 1) {
        printf("String index: %" PRId64 "is out of Bounds\n", original_idx);
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
        (step < 0 && (idx1 < idx2 || idx2 >= s_len-1))) {
        return s;
    }

    char* dest_char = (char*)malloc(s_len + 1);
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
    uint8_t extended_ascii = (uint8_t)val;
    dest_char[0] = extended_ascii;
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


// size_plus_one is the size of the string including the null character
LFORTRAN_API void _lfortran_string_init(int64_t size_plus_one, char *s) {
    int size = size_plus_one-1;
    for (int i=0; i < size; i++) {
        s[i] = ' ';
    }
    s[size] = '\0';
}

// bit  ------------------------------------------------------------------------

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

LFORTRAN_API int32_t _lfortran_ibits32(int32_t i, int32_t pos, int32_t len) {
    uint32_t ui = i;
    return ((ui << (BITS_32 - pos - len)) >> (BITS_32 - len));
}

LFORTRAN_API int64_t _lfortran_ibits64(int64_t i, int32_t pos, int32_t len) {
    uint64_t ui = i;
    return ((ui << (BITS_64 - pos - len)) >> (BITS_64 - len));
}

// cpu_time  -------------------------------------------------------------------

LFORTRAN_API double _lfortran_d_cpu_time() {
    return ((double) clock()) / CLOCKS_PER_SEC;
}

LFORTRAN_API float _lfortran_s_cpu_time() {
    return ((float) clock()) / CLOCKS_PER_SEC;
}

// system_time -----------------------------------------------------------------

LFORTRAN_API int32_t _lfortran_i32sys_clock_count() {
#if defined(_WIN32)
    return - INT_MAX;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (int32_t)(ts.tv_nsec / 1000000) + ((int32_t)ts.tv_sec * 1000);
    } else {
        return - INT_MAX;
    }
#endif
}

LFORTRAN_API int32_t _lfortran_i32sys_clock_count_rate() {
#if defined(_WIN32)
    return - INT_MAX;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return 1e3; // milliseconds
    } else {
        return 0;
    }
#endif
}

LFORTRAN_API int32_t _lfortran_i32sys_clock_count_max() {
#if defined(_WIN32)
    return 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return INT_MAX;
    } else {
        return 0;
    }
#endif
}

LFORTRAN_API uint64_t _lfortran_i64sys_clock_count() {
#if defined(_WIN32)
    return 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)(ts.tv_nsec) + ((uint64_t)ts.tv_sec * 1000000000);
    } else {
        return - LLONG_MAX;
    }
#endif
}

LFORTRAN_API int64_t _lfortran_i64sys_clock_count_rate() {
#if defined(_WIN32)
    return 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        // FIXME: Rate can be in microseconds or nanoseconds depending on
        //          resolution of the underlying platform clock.
        return 1e9; // nanoseconds
    } else {
        return 0;
    }
#endif
}

LFORTRAN_API int64_t _lfortran_i64sys_clock_count_max() {
#if defined(_WIN32)
    return 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return LLONG_MAX;
    } else {
        return 0;
    }
#endif
}

LFORTRAN_API float _lfortran_i32r32sys_clock_count_rate() {
#if defined(_WIN32)
    return 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return 1e3; // milliseconds
    } else {
        return 0;
    }
#endif
}

LFORTRAN_API double _lfortran_i64r64sys_clock_count_rate() {
#if defined(_WIN32)
    return 0;
#else
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return 1e9; // nanoseconds
    } else {
        return 0;
    }
#endif
}

LFORTRAN_API char* _lfortran_zone() {
    char* result = (char*)malloc(12 * sizeof(char)); // "(+|-)hhmm\0" = 5 + 1

    if (result == NULL) {
        return NULL;
    }

#if defined(_WIN32)
    // Windows doesn't provide timezone offset directly, so we calculate it
    TIME_ZONE_INFORMATION tzinfo;
    DWORD retval = GetTimeZoneInformation(&tzinfo);
    
    // Calculate the total offset in minutes
    int offset_minutes = -tzinfo.Bias; // Bias is in minutes; negative for UTC+
    
    if (retval == TIME_ZONE_ID_DAYLIGHT) {
        offset_minutes -= tzinfo.DaylightBias; // Apply daylight saving if applicable
    } else if (retval == TIME_ZONE_ID_STANDARD) {
        offset_minutes -= tzinfo.StandardBias; // Apply standard bias if applicable
    }

#elif defined(__APPLE__) && !defined(__aarch64__)
    // For non-ARM-based Apple platforms
    time_t t = time(NULL);
    struct tm* ptm = localtime(&t);

    // The tm_gmtoff field holds the time zone offset in seconds
    long offset_seconds = ptm->tm_gmtoff;
    int offset_minutes = offset_seconds / 60;

#else
    // For Linux and other platforms
    time_t t = time(NULL);
    struct tm* ptm = localtime(&t);

    // The tm_gmtoff field holds the time zone offset in seconds
    long offset_seconds = ptm->tm_gmtoff;
    int offset_minutes = offset_seconds / 60;
#endif
    char sign = offset_minutes >= 0 ? '+' : '-';
    int offset_hours = abs(offset_minutes / 60);
    int remaining_minutes = abs(offset_minutes % 60);
    snprintf(result, 12, "%c%02d%02d", sign, offset_hours, remaining_minutes);
    return result;
}

LFORTRAN_API char* _lfortran_time() {
    char* result = (char*)malloc(13 * sizeof(char)); // "hhmmss.sss\0" = 12 + 1

    if (result == NULL) {
        return NULL;
    }

#if defined(_WIN32)
    SYSTEMTIME st;
    GetLocalTime(&st); // Gets the current local time
    sprintf(result, "%02d%02d%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#elif defined(__APPLE__) && !defined(__aarch64__)
    // For non-ARM-based Apple platforms, use current time functions
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* ptm = localtime(&tv.tv_sec);
    int milliseconds = tv.tv_usec / 1000;
    sprintf(result, "%02d%02d%02d.%03d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec, milliseconds);
#else
    // For Linux and other platforms
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* ptm = localtime(&ts.tv_sec);
    int milliseconds = ts.tv_nsec / 1000000;
    sprintf(result, "%02d%02d%02d.%03d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec, milliseconds);
#endif
    return result;
}

LFORTRAN_API char* _lfortran_date() {
    // Allocate memory for the output string (8 characters minimum)
    char* result = (char*)malloc(32 * sizeof(char)); // "ccyymmdd\0" = 8 + 1

    if (result == NULL) {
        return NULL; // Handle memory allocation failure
    }

#if defined(_WIN32)
    SYSTEMTIME st;
    GetLocalTime(&st); // Get the current local date
    sprintf(result, "%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
#elif defined(__APPLE__) && !defined(__aarch64__)
    // For non-ARM-based Apple platforms
    time_t t = time(NULL);
    struct tm* ptm = localtime(&t);
    sprintf(result, "%04d%02d%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
#else
    // For Linux and other platforms
    time_t t = time(NULL);
    struct tm* ptm = localtime(&t);
    snprintf(result, 32, "%04d%02d%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
#endif

    return result; // Return the formatted date string
}

LFORTRAN_API int32_t _lfortran_values(int32_t n)
{   int32_t result = 0;
#if defined(_WIN32)
    SYSTEMTIME st;
    GetLocalTime(&st); // Get the current local date
    if (n == 1) result = st.wYear;
    else if (n == 2) result = st.wMonth;
    else if (n == 3) result = st.wDay;
    else if (n == 4) result = 330;
    else if (n == 5) result = st.wHour;
    else if (n == 6) result = st.wMinute;
    else if (n == 7) result = st.wSecond;
    else if (n == 8) result = st.wMilliseconds;
#elif defined(__APPLE__) && !defined(__aarch64__)
    // For non-ARM-based Apple platforms
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* ptm = localtime(&tv.tv_sec);
    int milliseconds = tv.tv_usec / 1000;
    if (n == 1) result = ptm->tm_year + 1900;
    else if (n == 2) result = ptm->tm_mon + 1;
    else if (n == 3) result = ptm->tm_mday;
    else if (n == 4) result = 330;
    else if (n == 5) result = ptm->tm_hour;
    else if (n == 6) result = ptm->tm_min;
    else if (n == 7) result = ptm->tm_sec;
    else if (n == 8) result = milliseconds;
#else
    // For Linux and other platforms
    time_t t = time(NULL);
    struct tm* ptm = localtime(&t);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    if (n == 1) result = ptm->tm_year + 1900;
    else if (n == 2) result = ptm->tm_mon + 1;
    else if (n == 3) result = ptm->tm_mday;
    else if (n == 4) result = 330;
    else if (n == 5) result = ptm->tm_hour;
    else if (n == 6) result = ptm->tm_min;
    else if (n == 7) result = ptm->tm_sec;
    else if (n == 8) result = ts.tv_nsec / 1000000;
#endif
    return result;
}

LFORTRAN_API float _lfortran_sp_rand_num() {
    return rand() / (float) RAND_MAX;
}

LFORTRAN_API double _lfortran_dp_rand_num() {
    return rand() / (double) RAND_MAX;
}

LFORTRAN_API int32_t _lfortran_int32_rand_num() {
    return rand();
}

LFORTRAN_API int64_t _lfortran_int64_rand_num() {
    return rand();
}

LFORTRAN_API bool _lfortran_random_init(bool repeatable, bool image_distinct) {
    if (repeatable) {
            srand(0);
    } else {
        srand(time(NULL));
    }
    return false;
}

LFORTRAN_API int64_t _lfortran_random_seed(unsigned seed)
{
    srand(seed);
    // The seed array size is typically 8 elements because Fortran's RNG often uses a seed with a fixed length of 8 integers to ensure sufficient randomness and repeatability in generating sequences of random numbers.
    return 8;

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

#define MAXUNITS 1000

struct UNIT_FILE {
    int32_t unit;
    char* filename;
    FILE* filep;
    bool unit_file_bin;
};

int32_t last_index_used = -1;

struct UNIT_FILE unit_to_file[MAXUNITS];

void store_unit_file(int32_t unit_num, char* filename, FILE* filep, bool unit_file_bin) {
    for( int i = 0; i <= last_index_used; i++ ) {
        if( unit_to_file[i].unit == unit_num ) {
            unit_to_file[i].unit = unit_num;
            unit_to_file[i].filep = filep;
            unit_to_file[i].unit_file_bin = unit_file_bin;
        }
    }
    last_index_used += 1;
    if( last_index_used >= MAXUNITS ) {
        printf("Only %d units can be opened for now\n.", MAXUNITS);
        exit(1);
    }
    unit_to_file[last_index_used].unit = unit_num;
    unit_to_file[last_index_used].filename = filename;
    unit_to_file[last_index_used].filep = filep;
    unit_to_file[last_index_used].unit_file_bin = unit_file_bin;
}

FILE* get_file_pointer_from_unit(int32_t unit_num, bool *unit_file_bin) {
    *unit_file_bin = false;
    for( int i = 0; i <= last_index_used; i++ ) {
        if( unit_to_file[i].unit == unit_num ) {
            *unit_file_bin = unit_to_file[i].unit_file_bin;
            return unit_to_file[i].filep;
        }
    }
    return NULL;
}

char* get_file_name_from_unit(int32_t unit_num, bool *unit_file_bin) {
    *unit_file_bin = false;
    for (int i = 0; i <= last_index_used; i++) {
        if (unit_to_file[i].unit == unit_num) {
            *unit_file_bin = unit_to_file[i].unit_file_bin;
            return unit_to_file[i].filename;
        }
    }
    return NULL;
}

void remove_from_unit_to_file(int32_t unit_num) {
    int index = -1;
    for( int i = 0; i <= last_index_used; i++ ) {
        if( unit_to_file[i].unit == unit_num ) {
            index = i;
            break;
        }
    }
    if( index == -1 ) {
        return ;
    }
    for( int i = index; i < last_index_used; i++ ) {
        unit_to_file[i].unit = unit_to_file[i + 1].unit;
        unit_to_file[i].filename = unit_to_file[i + 1].filename;
        unit_to_file[i].filep = unit_to_file[i + 1].filep;
        unit_to_file[i].unit_file_bin = unit_to_file[i + 1].unit_file_bin;
    }
    last_index_used -= 1;
}

LFORTRAN_API int64_t _lfortran_open(int32_t unit_num, char *f_name, char *status, char *form)
{
    if (f_name == NULL) {
        f_name = "_lfortran_generated_file.txt";
    }

    if (status == NULL) {
        status = "unknown";
    }

    if (form == NULL) {
        form = "formatted";
    }
    bool file_exists[1] = {false};

    size_t len = strlen(f_name);
    if (*(f_name + len - 1) == ' ') {
        // trim trailing spaces
        char* end = f_name + len - 1;
        while (end > f_name && isspace((unsigned char) *end)) {
            end--;
        }
        *(end + 1) = '\0';
    }

    _lfortran_inquire(f_name, file_exists, -1, NULL, NULL);
    char *access_mode = NULL;
    /*
     STATUS=`specifier` in the OPEN statement
     The following are the available specifiers:
     * "old"     (file must already exist)
     * "new"     (file does not exist and will be created)
     * "scratch" (temporary file will be deleted when closed)
     * "replace" (file will be created, replacing any existing file)
     * "unknown" (it is not known whether the file exists)
     */
    if (streql(status, "old")) {
        if (!*file_exists) {
            printf("Runtime error: File `%s` does not exists!\nCannot open a "
                "file with the `status=old`\n", f_name);
            exit(1);
        }
        access_mode = "r+";
    } else if (streql(status, "new")) {
        if (*file_exists) {
            printf("Runtime error: File `%s` exists!\nCannot open a file with "
                "the `status=new`\n", f_name);
            exit(1);
        }
        access_mode = "w+";
    } else if (streql(status, "replace")) {
        access_mode = "w+";
    } else if (streql(status, "unknown")) {
        if (!*file_exists) {
            FILE *fd = fopen(f_name, "w");
            if (fd) {
                fclose(fd);
            }
        }
        access_mode = "r+";
    } else if (streql(status, "scratch")) {
        printf("Runtime error: Unhandled type status=`scratch`\n");
        exit(1);
    } else {
        printf("Runtime error: STATUS specifier in OPEN statement has "
            "invalid value '%s'\n", status);
        exit(1);
    }

    bool unit_file_bin;
    if (streql(form, "formatted")) {
        unit_file_bin = false;
    } else if (streql(form, "unformatted")) {
        unit_file_bin = true;
    } else {
        printf("Runtime error: FORM specifier in OPEN statement has "
            "invalid value '%s'\n", form);
        exit(1);
    }

    FILE *fd = fopen(f_name, access_mode);
    if (!fd)
    {
        printf("Runtime error: Error in opening the file!\n");
        perror(f_name);
        exit(1);
    }
    store_unit_file(unit_num, f_name, fd, unit_file_bin);
    return (int64_t)fd;
}

LFORTRAN_API void _lfortran_flush(int32_t unit_num)
{
    // special case: flush all open units
    if (unit_num == -1) {
        for (int i = 0; i <= last_index_used; i++) {
            if (unit_to_file[i].filep != NULL) {
                fflush(unit_to_file[i].filep);
            }
        }
    } else {
        bool unit_file_bin;
        FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
        if( filep == NULL ) {
            if ( unit_num == 6 ) {
                // special case: flush OUTPUT_UNIT
                fflush(stdout);
                return;
            } else if ( unit_num == 5 ) {
                // special case: flush INPUT_UNIT
                fflush(stdin);
                return;
            } else if ( unit_num == 0 ) {
                // special case: flush ERROR_UNIT
                fflush(stderr);
                return;
            }
            printf("Specified UNIT %d in FLUSH is not connected.\n", unit_num);
            exit(1);
        }
        fflush(filep);
    }
}

LFORTRAN_API void _lfortran_inquire(char *f_name, bool *exists, int32_t unit_num, bool *opened, int32_t *size) {
    if (f_name && unit_num != -1) {
        printf("File name and file unit number cannot be specifed together.\n");
        exit(1);
    }
    if (f_name != NULL) {
        FILE *fp = fopen(f_name, "r");
        if (fp != NULL) {
            *exists = true;
            if (size != NULL) {
                fseek(fp, 0, SEEK_END);
                *size = ftell(fp);
            }
            fclose(fp); // close the file
            return;
        }
        *exists = false;
    }
    if (unit_num != -1) {
        bool unit_file_bin;
        if (get_file_pointer_from_unit(unit_num, &unit_file_bin) != NULL) {
            *opened = true;
        } else {
            *opened = false;
        }
    }
}

LFORTRAN_API void _lfortran_rewind(int32_t unit_num)
{
    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if( filep == NULL ) {
        printf("Specified UNIT %d in REWIND is not created or connected.\n", unit_num);
        exit(1);
    }
    rewind(filep);
}

LFORTRAN_API void _lfortran_backspace(int32_t unit_num)
{
    bool unit_file_bin;
    FILE* fd = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if( fd == NULL ) {
        printf("Specified UNIT %d in BACKSPACE is not created or connected.\n",
            unit_num);
        exit(1);
    }
    int n = ftell(fd);
    for(int i = n; i >= 0; i --) {
        char c = fgetc(fd);
        if (i == n) {
            // Skip previous record newline
            fseek(fd, -3, SEEK_CUR);
            continue;
        } else  if (c == '\n') {
            break;
        } else {
            fseek(fd, -2, SEEK_CUR);
        }
    }
}

LFORTRAN_API void _lfortran_read_int32(int32_t *p, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        (void)!scanf("%d", p);
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (unit_file_bin) {
        (void)!fread(p, sizeof(*p), 1, filep);
    } else {
        (void)!fscanf(filep, "%d", p);
    }
}

LFORTRAN_API void _lfortran_read_int64(int64_t *p, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        (void)!scanf("%" PRId64, p);
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (unit_file_bin) {
        (void)!fread(p, sizeof(*p), 1, filep);
    } else {
        (void)!fscanf(filep, "%" PRId64, p);
    }
}

LFORTRAN_API void _lfortran_read_array_int8(int8_t *p, int array_size, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        for (int i = 0; i < array_size; i++) {
            (void)!scanf("%s", &p[i]);
        }
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (unit_file_bin) {
        (void)!fread(p, sizeof(int8_t), array_size, filep);
    } else {
        for (int i = 0; i < array_size; i++) {
            (void)!fscanf(filep, "%s", &p[i]);
        }
    }
}

LFORTRAN_API void _lfortran_read_array_int32(int32_t *p, int array_size, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        for (int i = 0; i < array_size; i++) {
            (void)!scanf("%d", &p[i]);
        }
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (unit_file_bin) {
        (void)!fread(p, sizeof(int32_t), array_size, filep);
    } else {
        for (int i = 0; i < array_size; i++) {
            (void)!fscanf(filep, "%d", &p[i]);
        }
    }
}

LFORTRAN_API void _lfortran_read_char(char **p, int32_t unit_num)
{
    const char SPACE = ' ';
    int n = strlen(*p);
    if (unit_num == -1) {
        // Read from stdin
        *p = (char*)malloc(n * sizeof(char));
        (void)!fgets(*p, n + 1, stdin);
        (*p)[strcspn(*p, "\n")] = 0;
        size_t input_length = strlen(*p);
        while (input_length < n) {
            strncat(*p, &SPACE, 1);
            input_length++;
        }
        (*p)[n] = '\0';
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (unit_file_bin) {
        // read the record marker for data length
        int32_t data_length;
        if (fread(&data_length, sizeof(int32_t), 1, filep) != 1) {
            printf("Error reading data length from file.\n");
            exit(1);
        }

        // allocate memory for the data based on data length
        *p = (char*)malloc((data_length + 1) * sizeof(char));
        if (*p == NULL) {
            printf("Memory allocation failed.\n");
            exit(1);
        }

        // read the actual data
        if (fread(*p, sizeof(char), data_length, filep) != data_length) {
            printf("Error reading data from file.\n");
            free(*p);
            exit(1);
        }
        (*p)[data_length] = '\0';

        // read the record marker after data
        int32_t check_length;
        if (fread(&check_length, sizeof(int32_t), 1, filep) != 1) {
            printf("Error reading end data length from file.\n");
            free(*p);
            exit(1);
        }

        // verify that the start and end markers match
        if (check_length != data_length) {
            printf("Data length mismatch between start and end markers.\n");
            free(*p);
            exit(1);
        }
    } else {
        char *tmp_buffer = (char*)malloc((n + 1) * sizeof(char));
        (void)!fscanf(filep, "%s", tmp_buffer);
        size_t input_length = strlen(tmp_buffer);
        strcpy(*p, tmp_buffer);
        free(tmp_buffer);
        while (input_length < n) {
            strncat(*p, &SPACE, 1);
            input_length++;
        }
        (*p)[n] = '\0';
    }
    if (streql(*p, "")) {
        printf("Runtime error: End of file!\n");
        exit(1);
    }
}

LFORTRAN_API void _lfortran_read_float(float *p, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        (void)!scanf("%f", p);
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (unit_file_bin) {
        (void)!fread(p, sizeof(*p), 1, filep);
    } else {
        (void)!fscanf(filep, "%f", p);
    }
}

LFORTRAN_API void _lfortran_read_array_float(float *p, int array_size, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        for (int i = 0; i < array_size; i++) {
            (void)!scanf("%f", &p[i]);
        }
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (unit_file_bin) {
        (void)!fread(p, sizeof(float), array_size, filep);
    } else {
        for (int i = 0; i < array_size; i++) {
            (void)!fscanf(filep, "%f", &p[i]);
        }
    }
}

LFORTRAN_API void _lfortran_read_array_double(double *p, int array_size, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        for (int i = 0; i < array_size; i++) {
            (void)!scanf("%lf", &p[i]);
        }
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (unit_file_bin) {
        (void)!fread(p, sizeof(double), array_size, filep);
    } else {
        for (int i = 0; i < array_size; i++) {
            (void)!fscanf(filep, "%lf", &p[i]);
        }
    }
}

LFORTRAN_API void _lfortran_read_array_char(char **p, int array_size, int32_t unit_num)
{
    // TODO: Add support for initializing character arrays to read more than one character
    int n = 1;
    const char SPACE = ' ';
    if (unit_num == -1) {
        // Read from stdin
        for (int i = 0; i < array_size; i++) {
            p[i] = (char*) malloc((n + 1) * sizeof(char));
            char *tmp_buffer = (char*)malloc((n + 1) * sizeof(char));
            (void)!fscanf(stdin, "%s", tmp_buffer);
            size_t input_length = strlen(tmp_buffer);
            strcpy(p[i], tmp_buffer);
            free(tmp_buffer);
            while (input_length < n) {
                strncat(p[i], &SPACE, 1);
                input_length++;
            }
            p[i][n] = '\0';
        }
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    for (int i = 0; i < array_size; i++) {
        p[i] = (char*) malloc((n + 1) * sizeof(char));
        if (unit_file_bin) {
            (void)!fread(p[i], sizeof(char), n, filep);
            p[i][1] = '\0';
        } else {
            char *tmp_buffer = (char*)malloc((n + 1) * sizeof(char));
            (void)!fscanf(filep, "%s", tmp_buffer);
            size_t input_length = strlen(tmp_buffer);
            strcpy(p[i], tmp_buffer);
            free(tmp_buffer);
            while (input_length < n) {
                strncat(p[i], &SPACE, 1);
                input_length++;
            }
            p[i][n] = '\0';
        }
    }
}

LFORTRAN_API void _lfortran_read_double(double *p, int32_t unit_num)
{
    if (unit_num == -1) {
        // Read from stdin
        (void)!scanf("%lf", p);
        return;
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (unit_file_bin) {
        (void)!fread(p, sizeof(*p), 1, filep);
    } else {
        (void)!fscanf(filep, "%lf", p);
    }
}

LFORTRAN_API void _lfortran_formatted_read(int32_t unit_num, int32_t* iostat, int32_t* chunk, char* fmt, int32_t no_of_args, ...)
{
    if (!streql(fmt, "(a)")) {
        printf("Only (a) supported as fmt currently");
        exit(1);
    }

    // For now, this supports reading a single argument of type string
    // TODO: Support more arguments and other types

    va_list args;
    va_start(args, no_of_args);
    char** arg = va_arg(args, char**);

    int n = strlen(*arg);
    *arg = (char*)malloc((n + 1) * sizeof(char));
    const char SPACE = ' ';

    if (unit_num == -1) {
        // Read from stdin
        char *buffer = (char*)malloc((n + 1) * sizeof(char));
        if (fgets(buffer, n + 1, stdin) == NULL) {
            *iostat = -1;
            va_end(args);
            free(buffer);
            return;
        } else {
            if (streql(buffer, "\n")) {
                *iostat = -2;
            } else {
                *iostat = 0;
            }

            size_t input_length = strcspn(buffer, "\n");

            *chunk = input_length;
            while (input_length < n) {
                buffer[input_length] = SPACE;
                input_length++;
            }
            strcpy(*arg, buffer);
            va_end(args);
            free(buffer);
        }
    }

    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    } else {
        char *buffer = (char*)malloc((n + 1) * sizeof(char));
        if (fgets(buffer, n + 1, filep) == NULL) {
            *iostat = -1;
            va_end(args);
            free(buffer);
            return;
        } else {
            if (streql(buffer, "\n")) {
                *iostat = -2;
            } else {
                *iostat = 0;
            }

            (buffer)[strcspn(buffer, "\n")] = 0;

            size_t input_length = strlen(buffer);
            *chunk = input_length;
            while (input_length < n) {
                strncat(buffer, &SPACE, 1);
                input_length++;
            }
            strcpy(*arg, buffer);
            va_end(args);
            free(buffer);
        }
    }
}

LFORTRAN_API void _lfortran_empty_read(int32_t unit_num, int32_t* iostat) {
    if (unit_num == -1) {
        // Read from stdin
        return;
    }

    bool unit_file_bin;
    FILE* fp = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!fp) {
        printf("No file found with given unit\n");
        exit(1);
    }

    if (!unit_file_bin) {
        // The contents of `c` are ignored
        char c = fgetc(fp);
        while (c != '\n' && c != EOF) {
            c = fgetc(fp);
        }

        if (feof(fp)) {
            *iostat = -1;
        } else if (ferror(fp)) {
            *iostat = 1;
        } else {
            *iostat = 0;
        }
    }
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

LFORTRAN_API void _lfortran_file_write(int32_t unit_num, int32_t* iostat, const char *format, ...)
{
    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        filep = stdout;
    }
    va_list args;
    va_start(args, format);
    char* str = va_arg(args, char*);
    // Detect "\b" to raise error
    if(str[0] == '\b'){
        if(iostat == NULL){
            str = str+1;
            fprintf(stderr, "%s",str);
            exit(1);
        } else { // Delegate error handling to the user.
            *iostat = 11;
            return;
        }
    }
    if (unit_file_bin) {
        // size the size of `str_len` to bytes
        size_t str_len = strlen(str);

        // calculate record marker size
        int32_t record_marker = (int32_t)str_len;

        // write record marker before the data
        fwrite(&record_marker, sizeof(record_marker), 1, filep);

        size_t written = fwrite(str, sizeof(char), str_len, filep); // write as binary data

        // write the record marker after the data
        fwrite(&record_marker, sizeof(record_marker), 1, filep);

        if (written != str_len) {
            printf("Error writing data to file.\n");
            // TODO: not sure what is the right value of "iostat" in this case
            // it should be a positive value unique from other predefined iostat values
            // like IOSTAT_INQUIRE_INTERNAL_UNIT, IOSTAT_END, and IOSTAT_EOR.
            // currently, I've set it to 11
            if(iostat != NULL) *iostat = 11;
            exit(1);
        } else {
            if(iostat != NULL) *iostat = 0;
        }
    } else {
        if(strcmp(format, "%s%s") == 0){
            char* end = va_arg(args, char*);
            fprintf(filep, format, str, end);
        } else {
            fprintf(filep, format, str);
        }
        if(iostat != NULL) *iostat = 0;
    }
    va_end(args);
    (void)!ftruncate(fileno(filep), ftell(filep));
}

LFORTRAN_API void _lfortran_string_write(char **str_holder, int64_t* size, int64_t* capacity, int32_t* iostat, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char* str;
    char* end = "";
    if(strcmp(format, "%s%s") == 0){
        str = va_arg(args, char*);
        end = va_arg(args, char*);
    } else if(strcmp(format, "%s") == 0){
        str = va_arg(args, char*);
    } else {
        fprintf(stderr,"Compiler Error : Undefined Format");
        exit(1);
    }

    // Detect "\b" to raise error
    if(str[0] == '\b'){
        if(iostat == NULL){
            str = str+1;
            fprintf(stderr, "%s",str);
            exit(1);
        } else { // Delegate error handling to the user.
            *iostat = 11;
            return;
        }
    }

    char *s = (char *) malloc(strlen(str)*sizeof(char) + strlen(end)*sizeof(char) + 1);
    sprintf(s, format, str, end);

    if(((*size) == -1) && ((*capacity) == -1)){ 
        _lfortran_strcpy_pointer_string(str_holder, s);
    } else {
        _lfortran_strcpy_descriptor_string(str_holder, s, size, capacity);
    }
    free(s);
    va_end(args);
    if(iostat != NULL) *iostat = 0;
}

LFORTRAN_API void _lfortran_string_read_i32(char *str, char *format, int32_t *i) {
    sscanf(str, format, i);
}

LFORTRAN_API void _lfortran_string_read_i64(char *str, char *format, int64_t *i) {
    sscanf(str, format, i);
}

LFORTRAN_API void _lfortran_string_read_f32(char *str, char *format, float *f) {
    sscanf(str, format, f);
}

LFORTRAN_API void _lfortran_string_read_f64(char *str, char *format, double *f) {
    sscanf(str, format, f);
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

LFORTRAN_API void _lfortran_string_read_str(char *str, char *format, char **s) {
    char* without_whitespace_str = remove_whitespace(str);
    if (without_whitespace_str[0] == '\'' && without_whitespace_str[1] == '\'' &&
        without_whitespace_str[2] == '\0'
    ) {
        *s = strdup("");
    } else {
        sscanf(str, format, *s);
    }
}

LFORTRAN_API void _lfortran_string_read_bool(char *str, char *format, int32_t *i) {
    sscanf(str, format, i);
    printf("%s\n", str);
}

void lfortran_error(const char *message) {
    fprintf(stderr, "LFORTRAN ERROR: %s\n", message);
    exit(EXIT_FAILURE);
}

// TODO: add support for reading comma separated string, into `_arr` functions
// by accepting array size as an argument as well
LFORTRAN_API void _lfortran_string_read_i32_array(char *str, char *format, int32_t *arr) {
    lfortran_error("Reading into an array of int32_t is not supported.");
}

LFORTRAN_API void _lfortran_string_read_i64_array(char *str, char *format, int64_t *arr) {
    lfortran_error("Reading into an array of int64_t is not supported.");
}

LFORTRAN_API void _lfortran_string_read_f32_array(char *str, char *format, float *arr) {
    lfortran_error("Reading into an array of float is not supported.");
}

LFORTRAN_API void _lfortran_string_read_f64_array(char *str, char *format, double *arr) {
    lfortran_error("Reading into an array of double is not supported.");
}

LFORTRAN_API void _lfortran_string_read_str_array(char *str, char *format, char **arr) {
    lfortran_error("Reading into an array of strings is not supported.");
}

LFORTRAN_API void _lpython_close(int64_t fd)
{
    if (fclose((FILE*)fd) != 0)
    {
        printf("Error in closing the file!\n");
        exit(1);
    }
}

LFORTRAN_API void _lfortran_close(int32_t unit_num, char* status)
{
    bool unit_file_bin;
    FILE* filep = get_file_pointer_from_unit(unit_num, &unit_file_bin);
    if (!filep) {
        printf("No file found with given unit\n");
        exit(1);
    }
    if (fclose(filep) != 0) {
        printf("Error in closing the file!\n");
        exit(1);
    }
    // TODO: Support other `status` specifiers
    if (status && strcmp(status, "delete") == 0) {
        if (remove(get_file_name_from_unit(unit_num, &unit_file_bin)) != 0) {
            printf("Error in deleting file!\n");
            exit(1);
        }
    }
    remove_from_unit_to_file(unit_num);
}

LFORTRAN_API int32_t _lfortran_ichar(char *c) {
     return (int32_t) c[0];
}

LFORTRAN_API int32_t _lfortran_iachar(char *c) {
    return (int32_t) (uint8_t)(c[0]);
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

LFORTRAN_API void _lpython_free_argv() {
    if (_argv != NULL) {
        for (size_t i = 0; i < _argc; i++) {
            free(_argv[i]);
        }
        free(_argv);
        _argv = NULL;
    }
}

LFORTRAN_API int32_t _lpython_get_argc() {
    return _argc;
}

LFORTRAN_API int32_t _lfortran_get_argc() {
    return _argc - 1;
}

LFORTRAN_API char *_lpython_get_argv(int32_t index) {
    return _argv[index];
}

// get_command_argument
LFORTRAN_API char *_lfortran_get_command_argument_value(int n) {
    if (n >= 0 && n < _argc) {
        return strdup(_argv[n]);  // Return a copy of the nth argument
    } else {
        return "";
    }
}

LFORTRAN_API int32_t _lfortran_get_command_argument_length(int n) {
    char* out = _lfortran_get_command_argument_value(n);
    return strlen(out);
}

LFORTRAN_API int32_t _lfortran_get_command_argument_status() {
    return 0;
}

// get_command
LFORTRAN_API char *_lfortran_get_command_command() {
    char* out;
    for(int i=0; i<_argc; i++) {
        if(i == 0) {
            out = strdup(_argv[i]);
        } else {
            out = realloc(out, strlen(out) + strlen(_argv[i]) + 1);
            strcat(out, " ");
            strcat(out, _argv[i]);
        }
    }
    return out;
}

LFORTRAN_API int32_t _lfortran_get_command_length() {
    char* out = _lfortran_get_command_command();
    return strlen(out);
}

LFORTRAN_API int32_t _lfortran_get_command_status() {
    return 1;
}

// << Command line arguments << ------------------------------------------------

// Initial setup
LFORTRAN_API void _lpython_call_initial_functions(int32_t argc_1, char *argv_1[]) {
    _lpython_set_argv(argc_1, argv_1);
    _lfortran_init_random_clock();
}

LFORTRAN_API int32_t _lfortran_command_argument_count() {
    return _argc - 1;
}
// << Initial setup << ---------------------------------------------------------

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
    // Assuming filename always has an extensions
    size_t end = strrchr(filename, '.')-filename-1;
    // Check for directories else start at 0th index
    char *slash_idx_ptr = strrchr(filename, '/');
    size_t start = 0;
    if (slash_idx_ptr) {
        start = slash_idx_ptr - filename+1;
    }
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
    char *filename = malloc(strlen(base_name) + 15);
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

#endif // HAVE_RUNTIME_STACKTRACE

LFORTRAN_API void print_stacktrace_addresses(char *filename, bool use_colors) {
#ifdef HAVE_RUNTIME_STACKTRACE
    source_filename = filename;
    struct Stacktrace d = get_stacktrace_addresses();
    get_local_address(&d);
    get_local_info_dwarfdump(&d);

#ifdef HAVE_LFORTRAN_MACHO
    for (int32_t i = d.local_pc_size-1; i >= 0; i--) {
#else
    for (int32_t i = d.local_pc_size-2; i >= 0; i--) {
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

LFORTRAN_API char *_lfortran_get_environment_variable(char *name) {
    // temporary solution, the below function _lfortran_get_env_variable should be used
    if (name == NULL) {
        return NULL;
    } else {
        // if the name is not found, return empty string
        char* empty_string = "";
        return getenv(name) ? getenv(name) : empty_string;
    }
}

LFORTRAN_API char *_lfortran_get_env_variable(char *name) {
    return getenv(name);
}

LFORTRAN_API int _lfortran_exec_command(char *cmd) {
    return system(cmd);
}
