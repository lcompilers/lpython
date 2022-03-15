from ltypes import i32, f64, c64
#from sys import exit


def ord(s: str) -> i32:
    """
    Returns an integer representing the Unicode code
    point of a given unicode character. This is the inverse of `chr()`.
    """
    if s == '0':
        return 48
    elif s == '1':
        return 49
#    else:
#        exit(1)


def chr(i: i32) -> str:
    """
    Returns the string representing a unicode character from
    the given Unicode code point. This is the inverse of `ord()`.
    """
    if i == 48:
        return '0'
    elif i == 49:
        return '1'
#    else:
#        exit(1)


# This is an implementation for f64.
# TODO: implement abs() as a generic procedure, and implement for all types
def abs(x: f64) -> f64:
    """
    Return the absolute value of `x`.
    """
    if x >= 0.0:
        return x
    else:
        return -x


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
    pos_to_str: list[str]
    pos_to_str = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
    while x > 0:
        rev_result += pos_to_str[x - (x//10)*10]
        rev_result_len += 1
        x = x//10
    pos: i32
    for pos in range(rev_result_len - 1, -1, -1):
        result += rev_result[pos]
    return result


def bool(x: i32) -> bool:
    """
    Return False when the argument `x` is 0, True otherwise.
    """
    return x != 0


def len(s: str) -> i32:
    """
    Return the length of the string `s`.
    """
    pass


def pow(x: i32, y: i32) -> f64:
    """
    Returns x**y.
    """
    return 1.0*x**y


def int(f: f64) -> i32:
    """
    Converts a floating point number to an integer.
    """
    pass # handled by LLVM


def float(i: i32) -> f64:
    """
    Converts an integer to a floating point number.
    """
    pass # handled by LLVM


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
    res += '0' if (n - (n//2)*2) == 0 else '1'
    while n > 1:
        n = n//2
        res += '0' if (n - (n//2)*2) == 0 else '1'
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
        remainder = n - (n//16)*16
        n -= remainder
        n = n//16
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
        remainder = n - (n//8)*8
        n -= remainder
        n = n//8
        res += _values[remainder]
    return prep + res[::-1]


def round(value: f64) -> i32:
    """
    Rounds a floating point number to the nearest integer.
    """
    if abs(value - int(value)) <= 0.5:
        return int(value)
    else:
        return int(value) + 1

def complex(x: f64, y: f64) -> c64:
    pass
