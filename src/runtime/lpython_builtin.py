from lpython import i8, i16, i32, i64, f32, f64, c32, c64, overload
#from sys import exit

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
    if x >= f32(0.0):
        return x
    else:
        return -x

@overload
def abs(x: i8) -> i8:
    if x >= i8(0):
        return x
    else:
        return -x

@overload
def abs(x: i16) -> i16:
    if x >= i16(0):
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
    if x >= i64(0):
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
    a = c.real
    b = _lfortran_caimag(c)
    return f32((a**f32(2) + b**f32(2))**f32(1/2))

@overload
def abs(c: c64) -> f64:
    a: f64
    b: f64
    a = c.real
    b = _lfortran_zaimag(c)
    return (a**2.0 + b**2.0)**(1/2)

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
def pow(x: i32, y: i32) -> f64:
    """
    Returns x**y.
    """
    return f64(x**y)

@overload
def pow(x: i64, y: i64) -> f64:
    return f64(x**y)

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
    return f32(x)**y

@overload
def pow(x: f32, y: i32) -> f32:
    return x**f32(y)

@overload
def pow(x: i32, y: f64) -> f64:
    return f64(x)**y

@overload
def pow(x: f64, y: i32) -> f64:
    return x**f64(y)

@overload
def pow(x: bool, y: bool) -> i32:
    if y and not x:
        return 0

    return 1

@overload
def pow(c: c32, y: i32) -> c32:
    return c**c32(y)

# sum
# supported data types: i32, i64, f32, f64

@overload
def sum(arr: list[i32]) -> i32:
    """
    Sum of the elements of `arr`.
    """
    sum: i32
    sum = 0

    i: i32
    for i in range(len(arr)):
        sum += arr[i]
    return sum

@overload
def sum(arr: list[i64]) -> i64:
    """
    Sum of the elements of `arr`.
    """
    sum: i64
    sum = i64(0)

    i: i32
    for i in range(len(arr)):
        sum += arr[i]
    return sum

@overload
def sum(arr: list[f32]) -> f32:
    """
    Sum of the elements of `arr`.
    """
    sum: f32
    sum = f32(0.0)

    i: i32
    for i in range(len(arr)):
        sum += arr[i]
    return sum

@overload
def sum(arr: list[f64]) -> f64:
    """
    Sum of the elements of `arr`.
    """
    sum: f64
    sum = 0.0

    i: i32
    for i in range(len(arr)):
        sum += arr[i]
    return sum

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
    i = i32(value)
    f: f64
    f = abs(value - f64(i))
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
    i = i32(value)
    f: f64
    f = f64(abs(value - f32(i)))
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
    return c64(0) + c64(0)*1j


@interface
@overload
def complex(x: f64) -> c64:
    return c64(x) + c64(0)*1j

@interface
@overload
def complex(x: i32) -> c32:
    return c32(x) + c32(0)*c32(1j)

@interface
@overload
def complex(x: f32) -> c32:
    return c32(x) + c32(0)*c32(1j)

@interface
@overload
def complex(x: i64) -> c64:
    return c64(x) + c64(0)*1j

@interface
@overload
def complex(x: f64, y: f64) -> c64:
    """
    Return a complex number with the given real and imaginary parts.
    """
    return c64(x) + c64(y)*1j

@interface
@overload
def complex(x: f32, y: f32) -> c32:
    return c32(x) + c32(y)*c32(1j)

@interface
@overload
def complex(x: f32, y: f64) -> c64:
    return c64(x) + c64(y)*1j

@interface
@overload
def complex(x: f64, y: f32) -> c64:
    return c64(x) + c64(y)*1j

@interface
@overload
def complex(x: i32, y: i32) -> c64:
    return c64(x) + c64(y)*1j

@interface
@overload
def complex(x: i64, y: i64) -> c64:
    return c64(x) + c64(y)*1j

@interface
@overload
def complex(x: i32, y: i64) -> c64:
    return c64(x) + c64(y)*1j

@interface
@overload
def complex(x: i64, y: i32) -> c64:
    return c64(x) + c64(y)*1j

@interface
@overload
def complex(x: i32, y: f64) -> c64:
    return c64(x) + c64(y)*1j

@interface
@overload
def complex(x: f64, y: i32) -> c64:
    return c64(x) + c64(y)*1j

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
    if r >= 0.0 or f64(result) == r:
        return float(result)
    return float(result - i64(1))


@overload
def _lpython_floordiv(a: f32, b: f32) -> f32:
    r: f64
    r = float(a)/float(b)
    result: i32
    resultf32: f32
    result = i32(r)
    if r >= 0.0 or f64(result) == r:
        resultf32 = f32(1.0) * f32(result)
    else:
        resultf32 = f32(1.0) * f32(result) - f32(1.0)
    return resultf32

@overload
def _lpython_floordiv(a: i8, b: i8) -> i8:
    r: f64 # f32 rounds things up and gives incorrect results
    r = float(a)/float(b)
    result: i8
    result = i8(r)
    if r >= 0.0 or f64(result) == r:
        return result
    return result - i8(1)

@overload
def _lpython_floordiv(a: i16, b: i16) -> i16:
    r: f64 # f32 rounds things up and gives incorrect results
    r = float(a)/float(b)
    result: i16
    result = i16(r)
    if r >= 0.0 or f64(result) == r:
        return result
    return result - i16(1)

@overload
def _lpython_floordiv(a: i32, b: i32) -> i32:
    r: f64 # f32 rounds things up and gives incorrect results
    r = float(a)/float(b)
    result: i32
    result = i32(r)
    if r >= 0.0 or f64(result) == r:
        return result
    return result - 1

@overload
def _lpython_floordiv(a: i64, b: i64) -> i64:
    r: f64
    r = a/b
    result: i64
    result = int(r)
    if r >= 0.0 or f64(result) == r:
        return result
    return result - i64(1)

@overload
def _lpython_floordiv(a: bool, b: bool) -> bool:
    if b == False:
        raise ValueError('Denominator cannot be False or 0.')
    return a


@overload
def _mod(a: i8, b: i8) -> i8:
    return a - _lpython_floordiv(a, b)*b

@overload
def _mod(a: i16, b: i16) -> i16:
    return a - _lpython_floordiv(a, b)*b

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
    if x >= f64(0) or x == f64(r):
        return r
    return r - i64(1)

@overload
def _floor(x: f32) -> i32:
    r: i32
    r = i32(x)
    if x >= f32(0) or x == f32(r):
        return r
    return r - 1


@overload
def _mod(a: i32, b: i32) -> i32:
    """
    Returns a%b
    """
    return a - i32(_floor(a/b))*b


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
    if y < i64(0):
        raise ValueError('y should be nonnegative')
    result: i64
    result = _mod(x**y, z)
    return result

@overload
def _lpython_str_capitalize(x: str) -> str:
    if len(x) == 0:
        return x
    i:str
    res:str = ""
    for i in x:
        if ord(i) >= 65 and ord(i) <= 90:  # Check if uppercase
            res += chr(ord(i) + 32)  # Convert to lowercase using ASCII values
        else:
            res += i

    val: i32
    val = ord(res[0])
    if val >= ord('a') and val <= ord('z'):
        val -= 32
    res = chr(val) + res[1:]
    return res

@overload
def _lpython_str_lower(x: str) -> str:
    res: str
    res = ""
    i:str
    for i in x:
        if ord('A') <= ord(i) and ord(i) <= ord('Z'):
            res += chr(ord(i) +32)
        else:
            res += i
    return res

@overload
def _lpython_str_upper(x: str) -> str:
    res: str
    res = ""
    i:str
    for i in x:
        if ord('a') <= ord(i) and ord(i) <= ord('z'):
            res += chr(ord(i) -32)
        else:
            res += i
    return res


@overload
def _lpython_str_find(s: str, sub: str) -> i32:
    s_len :i32; sub_len :i32; flag: bool; _len: i32;
    res: i32; i: i32;
    lps: list[i32] = []
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

@overload
def _lpython_str_endswith(s: str, suffix: str) -> bool: 

    if(len(suffix) > len(s)):
        return False
    
    i : i32
    i = 0
    while(i < len(suffix)):
        if(suffix[len(suffix) - i - 1] != s[len(s) - i - 1]):
            return False
        i += 1
        
    return True

@overload
def _lpython_str_partition(s:str, sep: str) -> tuple[str, str, str]:
    """
    Returns a 3-tuple splitted around seperator
    """
    if len(s) == 0:
        raise ValueError('empty string cannot be partitioned')
    if len(sep) == 0:
        raise ValueError('empty seperator')
    res : tuple[str, str, str]
    ind : i32
    ind = _lpython_str_find(s, sep)
    if ind == -1:
        res = (s, "", "")
    else: 
        res = (s[0:ind], sep, s[ind+len(sep): len(s)])    
    return res


def list(s: str) -> list[str]:
    l: list[str] = []
    i: i32
    if len(s) == 0:
        return l
    for i in range(len(s)):
        l.append(s[i])
    return l
