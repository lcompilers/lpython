from ltypes import i32
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
    if x == 0:
        return False
    else:
        return True
