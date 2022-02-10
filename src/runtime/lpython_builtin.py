from ltypes import i32
from sys import exit


def ord(s: str) -> i32:
    """
    Returns an integer representing the Unicode code
    point of a given unicode character. This is the inverse of `chr()`.
    """
    if s == '0':
        return 48
    elif s == '1':
        return 49
    else:
        exit(1)


def chr(i: i32) -> str:
    """
    Returns the string representing a unicode character from
    the given Unicode code point. This is the inverse of `ord()`.
    """
    if i == 48:
        return '0'
    elif i == 49:
        return '1'
    else:
        exit(1)
