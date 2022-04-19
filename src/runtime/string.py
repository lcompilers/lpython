from ltypes import i32, f64, f32, i64, ccall, c32, c64

whitespace: str = ' \t\n\r\v\f'
ascii_lowercase: str= 'abcdefghijklmnopqrstuvwxyz'
ascii_uppercase: str = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
digits: str = '0123456789'
hexdigits: str = digits + 'abcdef' + 'ABCDEF'
octdigits: str = '01234567'
punctuation: str = "#$%&'()*+,-./:;<=>?@[\]^_`{|}~"

def capitalize(s: str) -> str:
    """
    Return a copy of the string with its first character capitalized and the rest lowercased.
    """
    result: str
    result = s[0].upper() + s[1:]
    return result