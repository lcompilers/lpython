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
    if (n - _lpython_floordiv(n, 2)*2) == 0:
        res += '0'
    else:
        res += '1'
    while n > 1:
        n = _lpython_floordiv(n, 2)
        if (n - _lpython_floordiv(n, 2)*2) == 0:
            res += '0'
        else:
            res += '1'
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
    """
    Return the tuple (x//y, x%y).
    """
    if y == 0:
        raise ZeroDivisionError("Integer division or modulo by zero not possible")
    t: tuple[i32, i32]
    t = (_lpython_floordiv(x, y), _mod(x, y))
    return t


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
    r: f64
    r = float(a)/float(b)
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
    r: f64 # f32 rounds things up and gives incorrect results
    r = float(a)/float(b)
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


@overload
def _floor(x: f64) -> i64:
    r: i64
    r = int(x)
    if x >= 0 or x == r:
        return r
    return r - 1

@overload
def _floor(x: f32) -> i32:
    r: i32
    r = int(x)
    if x >= 0 or x == r:
        return r
    return r - 1


@overload
def _mod(a: i32, b: i32) -> i32:
    """
    Returns a%b
    """
    r: i32
    r = _floor(a/b)
    return a - r*b


@overload
def _mod(a: i64, b: i64) -> i64:
    """
    Returns a%b
    """
    r: i64
    r = _floor(a/b)
    return a - r*b


@overload
def pow(x: i32, y: i32, z: i32) -> i32:
    """
    Return `x` raised to the power `y`.
    """
    if y < 0:
        raise ValueError('y should be nonnegative')
    result: i32
    result = _mod(x**y, z)
    return result


@overload
def pow(x: i64, y: i64, z: i64) -> i64:
    """
    Return `x` raised to the power `y`.
    """
    if y < 0:
        raise ValueError('y should be nonnegative')
    result: i64
    result = _mod(x**y, z)
    return result

@overload
def _lpython_str_capitalize(x: str) -> str:
    if len(x) == 0:
        return x
    val: i32
    val = ord(x[0])
    if val >= ord('a') and val <= ord('x'):
        val -= 32
    x = chr(val) + x[1:]
    return x

@overload
def _lpython_str_lower(x: str) -> str:
    res: str
    res = ""
    i:str
    for i in x:
        if ord('A') <= ord(i) and ord('Z') >= ord(i):
            res += chr(ord(i) +32)
        else:
            res += i
    return res

@overload
def _lpython_str_find(s: str, sub: str) -> i32:
    s_len :i32; sub_len :i32; flag: bool; _len: i32;
    res: i32; i: i32;
    lps: list[i32]
    s_len = len(s)
    sub_len = len(sub)
    flag = False
    res = -1
    if s_len == 0 or sub_len == 0:
        return 0 if sub_len == 0 or (sub_len == s_len) else -1

    for i in range(sub_len):
        lps.append(0)

    i = 1
    _len = 0
    while i < sub_len:
        if sub[i] == sub[_len]:
            _len += 1
            lps[i] = _len
            i += 1
        else:
            if _len != 0:
                _len = lps[_len - 1]
            else:
                lps[i] = 0
                i += 1

    j: i32
    j = 0
    i = 0
    while (s_len - i) >= (sub_len - j) and not flag:
        if sub[j] == s[i]:
            i += 1
            j += 1
        if j == sub_len:
            res = i- j
            flag = True
            j = lps[j - 1]
        elif i < s_len and sub[j] != s[i]:
            if j != 0:
                j = lps[j - 1]
            else:
                i = i + 1

    return res

def _lpython_str_rstrip(x: str) -> str:
    ind: i32
    ind = len(x) - 1
    while ind >= 0 and x[ind] == ' ':
        ind -= 1
    return x[0: ind + 1]

@overload
def _lpython_str_lstrip(x: str) -> str:
    ind :i32
    ind = 0
    while ind < len(x) and x[ind] == ' ':
        ind += 1
    return x[ind :len(x)]

@overload
def _lpython_str_strip(x: str) -> str:
    res :str
    res = _lpython_str_lstrip(x)
    res = _lpython_str_rstrip(res)
    return res

@overload
def _lpython_str_swapcase(s: str) -> str:
    res :str = ""
    cur: str
    for cur in s:
        if ord(cur) >= ord('a') and ord(cur) <= ord('z'):
            res += chr(ord(cur) - ord('a') + ord('A'))
        elif ord(cur) >= ord('A') and ord(cur) <= ord('Z'):
            res += chr(ord(cur) - ord('A') + ord('a'))
        else:
            res += cur
    return res

@overload
def _lpython_str_startswith(s: str ,sub: str) -> bool:
    res :bool
    res = not (len(s) == 0 and len(sub) > 0)
    i: i32; j: i32
    i = 0; j = 0
    while (i < len(s)) and ((j < len(sub)) and res):
        res = res and (s[i] == sub[j])
        i += 1; j+=1
    if res:
        res = res and (j == len(sub))
    return res
