from ltypes import i8, i16, i32, i64, f32, f64, c32, c64, overload
#from sys import exit


def ord(s: str) -> i32: # currently supports characters with unicode value between 32 to 126
    """
    Returns an integer representing the Unicode code
    point of a given unicode character. This is the inverse of `chr()`.
    """
    if len(s) != 1:
        return -1 # not a character
    i: i32
    for i in range(32, 127):
        if chr(i) == s:
            return i


def chr(i: i32) -> str: # currently supports unicode values between 32 to 126
    """
    Returns the string representing a unicode character from
    the given Unicode code point. This is the inverse of `ord()`.
    """
    if i < 32 or i > 126:
        return "Not yet supported"
    all_chars: str
    all_chars = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'
    return all_chars[i - 32]


#: abs() as a generic procedure.
#: supported types for argument:
#: i8, i16, i32, i64, f32, f64, bool, c32, c64
@overload
def abs(x: f64) -> f64:
    """
    Return the absolute value of `x`.
    """
    result: f64
    if x >= 0.0:
        result = x
    else:
        result = -x
    return result

@overload
def abs(x: f32) -> f32:
    if x >= 0.0:
        return x
    else:
        return -x

@overload
def abs(x: i8) -> i8:
    if x >= 0:
        return x
    else:
        return -x

@overload
def abs(x: i16) -> i16:
    if x >= 0:
        return x
    else:
        return -x

@overload
def abs(x: i32) -> i32:
    if x >= 0:
        return x
    else:
        return -x

@overload
def abs(x: i64) -> i64:
    if x >= 0:
        return x
    else:
        return -x

@overload
def abs(b: bool) -> i32:
    if b:
        return 1
    else:
        return 0

@overload
def abs(c: c32) -> f32:
    a: f32
    b: f32
    result: f32
    a = c.real
    b = _lfortran_caimag(c)
    result = (a**2 + b**2)**(1/2)
    return result

@overload
def abs(c: c64) -> f64:
    a: f64
    b: f64
    result: f64
    a = c.real
    b = _lfortran_zaimag(c)
    result = (a**2 + b**2)**(1/2)
    return result

@overload
def str() -> str:
    return ''

@overload
def str(x: str) -> str:
    return x

@overload
def str(x: bool) -> str:
    if x:
        return "True"
    return "False"

@overload
def str(x: i32) -> str:
    """
    Return the string representation of an integer `x`.
    """
    if x == 0:
        return '0'
    result: str
    result = ''
    if x < 0:
        result += '-'
        x = -x
    rev_result: str
    rev_result = ''
    rev_result_len: i32
    rev_result_len = 0
    while x > 0:
        rev_result += chr(48 + (x - _lpython_floordiv(x, 10)*10))
        rev_result_len += 1
        x = _lpython_floordiv(x, 10)
    pos: i32
    for pos in range(rev_result_len - 1, 1, -1):
        result += rev_result[pos]
    return result

@interface
def len(s: str) -> i32:
    """
    Return the length of the string `s`.
    """
    pass

#: pow() as a generic procedure.
#: supported types for arguments:
#: (i32, i32), (i64, i64), (f64, f64),
#: (f32, f32), (i32, f64), (f64, i32),
#: (i32, f32), (f32, i32), (bool, bool), (c32, i32)
@overload
def pow(x: i32, y: i32) -> i32:
    """
    Returns x**y.
    """
    return x**y

@overload
def pow(x: i64, y: i64) -> i64:
    return x**y

@overload
def pow(x: f32, y: f32) -> f32:
    return x**y

@overload
def pow(x: f64, y: f64) -> f64:
    """
    Returns x**y.
    """
    return x**y

@overload
def pow(x: i32, y: f32) -> f32:
    return x**y

@overload
def pow(x: f32, y: i32) -> f32:
    return x**y

@overload
def pow(x: i32, y: f64) -> f64:
    return x**y

@overload
def pow(x: f64, y: i32) -> f64:
    return x**y

@overload
def pow(x: bool, y: bool) -> i32:
    if y and not x:
        return 0

    return 1

@overload
def pow(c: c32, y: i32) -> c32:
    return c**y


def bin(n: i32) -> str:
    """
    Returns the binary representation of an integer `n`.
    """
    if n == 0:
        return '0b0'
    prep: str
    prep = '0b'
    if n < 0:
        n = -n
        prep = '-0b'
    res: str
    res = ''
    res += '0' if (n - _lpython_floordiv(n, 2)*2) == 0 else '1'
    while n > 1:
        n = _lpython_floordiv(n, 2)
        res += '0' if (n - _lpython_floordiv(n, 2)*2) == 0 else '1'
    return prep + res[::-1]


def hex(n: i32) -> str:
    """
    Returns the hexadecimal representation of an integer `n`.
    """
    hex_values: list[str]
    hex_values = ['0', '1', '2', '3', '4', '5', '6', '7',
                  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f']
    if n == 0:
        return '0x0'
    prep: str
    prep = '0x'
    if n < 0:
        prep = '-0x'
        n = -n
    res: str
    res = ""
    remainder: i32
    while n > 0:
        remainder = n - _lpython_floordiv(n, 16)*16
        n -= remainder
        n = _lpython_floordiv(n, 16)
        res += hex_values[remainder]
    return prep + res[::-1]


def oct(n: i32) -> str:
    """
    Returns the octal representation of an integer `n`.
    """
    _values: list[str]
    _values = ['0', '1', '2', '3', '4', '5', '6', '7',
               '8', '9', 'a', 'b', 'c', 'd', 'e', 'f']
    if n == 0:
        return '0o0'
    prep: str
    prep = '0o'
    if n < 0:
        prep = '-0o'
        n = -n
    res: str
    res = ""
    remainder: i32
    while n > 0:
        remainder = n - _lpython_floordiv(n, 8)*8
        n -= remainder
        n = _lpython_floordiv(n, 8)
        res += _values[remainder]
    return prep + res[::-1]

#: round() as a generic procedure.
#: supported types for argument:
#: i8, i16, i32, i64, f32, f64, bool
@overload
def round(value: f64) -> i32:
    """
    Rounds a floating point number to the nearest integer.
    """
    i: i32
    i = int(value)
    f: f64
    f = abs(value - i)
    if f < 0.5:
        return i
    elif f > 0.5:
        return i + 1
    else:
        if i - _lpython_floordiv(i, 2) * 2 == 0:
            return i
        else:
            return i + 1

@overload
def round(value: f32) -> i32:
    i: i32
    i = int(value)
    f: f64
    f = abs(value - i)
    if f < 0.5:
        return i
    elif f > 0.5:
        return i + 1
    else:
        if i - _lpython_floordiv(i, 2) * 2 == 0:
            return i
        else:
            return i + 1

@overload
def round(value: i32) -> i32:
    return value

@overload
def round(value: i64) -> i64:
    return value

@overload
def round(value: i8) -> i8:
    return value

@overload
def round(value: i16) -> i16:
    return value

@overload
def round(b: bool) -> i32:
    return abs(b)

#: complex() as a generic procedure.
#: supported types for arguments:
#: (f64, f64), (f32, f64), (f64, f32), (f32, f32),
#: (i32, i32), (i64, i64), (i32, i64), (i64, i32)


@interface
@overload
def complex() -> c64:
    return 0 + 0*1j


@interface
@overload
def complex(x: f64) -> c64:
    return x + 0*1j

@interface
@overload
def complex(x: i32) -> c32:
    result: c32
    result = x + 0*1j
    return result

@interface
@overload
def complex(x: f32) -> c32:
    result: c32
    result = x + 0*1j
    return result

@interface
@overload
def complex(x: i64) -> c64:
    return x + 0*1j

@interface
@overload
def complex(x: f64, y: f64) -> c64:
    """
    Return a complex number with the given real and imaginary parts.
    """
    return x + y*1j

@interface
@overload
def complex(x: f32, y: f32) -> c32:
    result: c32
    result = x + y*1j
    return result

@interface
@overload
def complex(x: f32, y: f64) -> c64:
    return x + y*1j

@interface
@overload
def complex(x: f64, y: f32) -> c64:
    return x + y*1j

@interface
@overload
def complex(x: i32, y: i32) -> c64:
    return x + y*1j

@interface
@overload
def complex(x: i64, y: i64) -> c64:
    return x + y*1j

@interface
@overload
def complex(x: i32, y: i64) -> c64:
    return x + y*1j

@interface
@overload
def complex(x: i64, y: i32) -> c64:
    return x + y*1j

@interface
@overload
def complex(x: i32, y: f64) -> c64:
    return x + y*1j

@interface
@overload
def complex(x: f64, y: i32) -> c64:
    return x + y*1j


@interface
def divmod(x: i32, y: i32) -> tuple[i32, i32]:
    #: TODO: Implement once we have tuple support in the LLVM backend
    pass


def lbound(x: i32[:], dim: i32) -> i32:
    pass


def ubound(x: i32[:], dim: i32) -> i32:
    pass


@ccall
def _lfortran_caimag(x: c32) -> f32:
    pass

@ccall
def _lfortran_zaimag(x: c64) -> f64:
    pass

@overload
def _lpython_imag(x: c64) -> f64:
    return _lfortran_zaimag(x)

@overload
def _lpython_imag(x: c32) -> f32:
    return _lfortran_caimag(x)


@overload
def _lpython_floordiv(a: f64, b: f64) -> f64:
    r: f64
    r = a/b
    result: i64
    result = int(r)
    if r >= 0.0 or result == r:
        return float(result)
    return float(result-1)


@overload
def _lpython_floordiv(a: f32, b: f32) -> f32:
    r: f32
    r = a/b
    result: i32
    resultf32: f32
    result = int(r)
    if r >= 0.0 or result == r:
        resultf32 = 1.0 * result
    else:
        resultf32 = 1.0 * result-1
    return resultf32

@overload
def _lpython_floordiv(a: i32, b: i32) -> i32:
    r: f32
    r = a/b
    result: i32
    result = int(r)
    if r >= 0.0 or result == r:
        return result
    return result - 1

@overload
def _lpython_floordiv(a: i64, b: i64) -> i64:
    r: f64
    r = a/b
    result: i64
    result = int(r)
    if r >= 0.0 or result == r:
        return result
    return result - 1


@overload
def _mod(a: i32, b: i32) -> i32:
    return a - _lpython_floordiv(a, b)*b

@overload
def _mod(a: f32, b: f32) -> f32:
    return a - _lpython_floordiv(a, b)*b

@overload
def _mod(a: i64, b: i64) -> i64:
    return a - _lpython_floordiv(a, b)*b

@overload
def _mod(a: f64, b: f64) -> f64:
    return a - _lpython_floordiv(a, b)*b


@overload
def max(a: i32, b: i32) -> i32:
    if a > b:
        return a
    else:
        return b

@overload
def max(a: i32, b: i32, c: i32) -> i32:
    res: i32 = a
    if b > res:
        res = b
    if c > res:
        res = c
    return res

@overload
def max(a: f64, b: f64, c: f64) -> f64:
    res: f64 =a
    if b - res > 1e-6:
        res = b
    if c - res > 1e-6:
        res = c
    return res

@overload
def max(a: f64, b: f64) -> f64:
    if a - b > 1e-6:
        return a
    else:
        return b

@overload
def min(a: i32, b: i32) -> i32:
    if a < b:
        return a
    else:
        return b

@overload
def min(a: i32, b: i32, c: i32) -> i32:
    res: i32 = a
    if b < res:
        res = b
    if c < res:
        res = c
    return res

@overload
def min(a: f64, b: f64, c: f64) -> f64:
    res: f64 = a
    if res - b > 1e-6:
        res = b
    if res - c > 1e-6:
        res = c
    return res

@overload
def min(a: f64, b: f64) -> f64:
    if b - a > 1e-6:
        return a
    else:
        return b
