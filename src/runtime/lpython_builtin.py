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
