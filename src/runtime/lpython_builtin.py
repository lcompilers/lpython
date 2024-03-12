from lpython import (i8, i16, i32, i64, f32, f64, c32, c64, overload, u8,
                     u16, u32, u64)
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
    n_: i32
    n_ = n
    if n_ < 0:
        n_ = -n_
        prep = '-0b'
    res: str
    res = ''
    if (n_ - (n_ // 2)*2) == 0:
        res += '0'
    else:
        res += '1'
    while n_ > 1:
        n_ = (n_ // 2)
        if (n_ - (n_ // 2)*2) == 0:
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
    n_: i32
    n_ = n
    if n_ < 0:
        prep = '-0x'
        n_ = -n_
    res: str
    res = ""
    remainder: i32
    while n_ > 0:
        remainder = n_ - (n_ // 16)*16
        n_ -= remainder
        n_ = (n_ // 16)
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
    n_: i32
    n_ = n
    if n_ < 0:
        prep = '-0o'
        n_ = -n_
    res: str
    res = ""
    remainder: i32
    while n_ > 0:
        remainder = n_ - (n_ // 8)*8
        n_ -= remainder
        n_ = (n_ // 8)
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
        if i - (i // 2) * 2 == 0:
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
        if i - (i // 2) * 2 == 0:
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
    t = ((x // y), _mod(x, y))
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
def _mod(a: i8, b: i8) -> i8:
    return a - (a // b)*b

@overload
def _mod(a: i16, b: i16) -> i16:
    return a - (a // b)*b

@overload
def _mod(a: i32, b: i32) -> i32:
    return a - (a // b)*b

@overload
def _mod(a: u8, b: u8) -> u8:
    return a - (a // b)*b

@overload
def _mod(a: u16, b: u16) -> u16:
    return a - (a // b)*b

@overload
def _mod(a: u32, b: u32) -> u32:
    return a - (a // b)*b

@overload
def _mod(a: f32, b: f32) -> f32:
    return a - (a // b)*b

@overload
def _mod(a: u64, b: u64) -> u64:
    return a - (a // b)*b

@overload
def _mod(a: i64, b: i64) -> i64:
    return a - (a // b)*b

@overload
def _mod(a: f64, b: f64) -> f64:
    return a - (a // b)*b


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
def _lpython_str_count(s: str, sub: str) -> i32:
    s_len :i32; sub_len :i32; flag: bool; _len: i32;
    count: i32; i: i32;
    lps: list[i32] = []
    s_len = len(s)
    sub_len = len(sub)

    if sub_len == 0:
        return s_len + 1 

    count = 0

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
    while (s_len - i) >= (sub_len - j):
        if sub[j] == s[i]:
            i += 1
            j += 1
        if j == sub_len:
            count += 1
            j = lps[j - 1]
        elif i < s_len and sub[j] != s[i]:
            if j != 0:
                j = lps[j - 1]
            else:
                i = i + 1

    return count


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
def _lpython_str_join(s:str, lis:list[str]) -> str:
    if len(lis) == 0: return ""
    res:str = lis[0]
    i:i32
    for i in range(1, len(lis)):
        res += s + lis[i]
    return res

def _lpython_str_isalpha(s: str) -> bool:
    ch: str
    if len(s) == 0: return False
    for ch in s:
        ch_ord: i32 = ord(ch)
        if 65 <= ch_ord and ch_ord <= 90:
            continue
        if 97 <= ch_ord and ch_ord <= 122:
            continue
        return False
    return True

def _lpython_str_isalnum(s: str) -> bool:
    ch: str
    if len(s) == 0: return False
    for ch in s:
        ch_ord: i32 = ord(ch)
        if 65 <= ch_ord and ch_ord <= 90:
            continue
        if 97 <= ch_ord and ch_ord <= 122:
            continue
        if 48 <= ch_ord and ch_ord <= 57:
            continue
        return False
    return True

def _lpython_str_isnumeric(s: str) -> bool:
    ch: str
    if len(s) == 0: return False
    for ch in s:
        ch_ord: i32 = ord(ch)
        if 48 <= ch_ord and ch_ord <= 57:
            continue
        return False
    return True

def _lpython_str_title(s: str) -> str:
    result: str = ""
    capitalize_next: bool = True
    ch: str
    for ch in s:
        ch_ord: i32 = ord(ch)
        if ch_ord >= 97  and ch_ord <= 122:
            if capitalize_next:
                result += chr(ord(ch) - ord('a') + ord('A'))
                capitalize_next = False
            else:
                result += ch
        elif ch_ord >= 65 and ch_ord <= 90:
            if capitalize_next:
                result += ch
                capitalize_next = False
            else:
                result += chr(ord(ch) + ord('a') - ord('A'))
        else:
            result += ch
            capitalize_next = True

    return result

def _lpython_str_istitle(s: str) -> bool:
    length: i32 = len(s)

    if length == 0:
        return False  # Empty string is not in title case

    word_start: bool = True  # Flag to track the start of a word
    ch: str
    only_whitespace: bool = True 
    for ch in s:
        if ch.isalpha() and (ord('A') <= ord(ch) and ord(ch) <= ord('Z')):
            only_whitespace = False
            if word_start:
                word_start = False
            else:
                return False  # Found an uppercase character in the middle of a word

        elif ch.isalpha() and (ord('a') <= ord(ch) and ord(ch) <= ord('z')):
            only_whitespace = False
            if word_start:
                return False  # Found a lowercase character in the middle of a word
            word_start = False
        else:
            word_start = True

    return True if not only_whitespace else False



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
def _lpython_str_split(x: str) -> list[str]:
    sep: str = ' '
    res: list[str] = []
    start:i32 = 0
    ind: i32
    x_strip: str = _lpython_str_strip(x)
    if (x_strip == ""): 
        return res
    while True:
        while (start < len(x_strip) and x_strip[start] == ' '):
            start += 1
        ind = _lpython_str_find(x_strip[start:len(x_strip)], sep)
        if ind == -1:
            res.append(x_strip[start:len(x_strip)])
            break
        else:
            res.append(x_strip[start:start + ind])
            start += ind + len(sep)
    return res
    
@overload
def _lpython_str_split(x: str, sep:str) -> list[str]:
    if len(sep) == 0:
        raise ValueError('empty separator')
    res: list[str] = []
    start:i32 = 0
    ind: i32
    while True:
        ind = _lpython_str_find(x[start:len(x)], sep)
        if ind == -1:
            res.append(x[start:len(x)])
            break
        else:
            res.append(x[start:start + ind])
            start += ind + len(sep)
    return res

@overload
def _lpython_str_replace(x: str, old:str, new:str) -> str:
    return _lpython_str_replace(x, old, new, len(x))
    

@overload
def _lpython_str_replace(x: str, old:str, new:str, count: i32) -> str:
    if (old == ""):
        res1: str = ""
        s: str
        for s in x:
            res1 += new + s
        return res1 + new
    res: str = ""
    i: i32 = 0
    ind: i32 = -1
    l: i32 = len(new)
    lo: i32 = len(old)
    lx: i32 = len(x)
    c: i32 = 0
    t: i32 = -1

    while(c<count):
        t = _lpython_str_find(x[i:lx], old)
        if(t==-1):
            break
        ind = i + t
        res = res + x[i:ind] + new
        i = ind + lo
        c = c + 1
    res = res + x[i:lx]
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
        raise ValueError('empty separator')
    res : tuple[str, str, str]
    ind : i32
    ind = _lpython_str_find(s, sep)
    if ind == -1:
        res = (s, "", "")
    else:
        res = (s[0:ind], sep, s[ind+len(sep): len(s)])
    return res

@overload
def _lpython_str_islower(s: str) -> bool:
    is_cased_present: bool
    is_cased_present = False
    i:str
    for i in s:
        if (ord(i) >= 97 and ord(i) <= 122) or (ord(i) >= 65 and ord(i) <= 90): # Implies it is a cased letter
            is_cased_present = True
            if not(ord(i) >= 97 and ord(i) <= 122): # Not lowercase
                return False
    return is_cased_present

@overload
def _lpython_str_isupper(s: str) -> bool:
    is_cased_present: bool
    is_cased_present = False
    i:str
    for i in s:
        if (ord(i) >= 97 and ord(i) <= 122) or (ord(i) >= 65 and ord(i) <= 90): # Implies it is a cased letter
            is_cased_present = True
            if not(ord(i) >= 65 and ord(i) <= 90): # Not lowercase
                return False
    return is_cased_present

@overload
def _lpython_str_isdecimal(s: str) -> bool:
    if len(s) == 0:
        return False
    i:str
    for i in s:
        if (ord(i) < 48 or ord(i) > 57): # Implies it is not a digit
            return False
    return True

@overload
def _lpython_str_isascii(s: str) -> bool:
    if len(s) == 0:
        return True
    i: str
    for i in s:
        if ord(i) < 0 or ord(i) > 127:
            return False
    return True

def _lpython_str_isspace(s:str) -> bool:
    if len(s) == 0:
        return False
    ch: str 
    for ch in s:
        if ch != ' ' and ch != '\t' and ch != '\n' and ch != '\r' and ch != '\f' and ch != '\v':
            return False
    return True


def list(s: str) -> list[str]:
    l: list[str] = []
    i: i32
    if len(s) == 0:
        return l
    for i in range(len(s)):
        l.append(s[i])
    return l
