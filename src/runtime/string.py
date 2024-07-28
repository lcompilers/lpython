whitespace : str = ' \t\n\r\v\f'
ascii_lowercase : str = 'abcdefghijklmnopqrstuvwxyz'
ascii_uppercase : str  = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
ascii_letters : str  = ascii_lowercase + ascii_uppercase
digits : str = '0123456789'
hexdigits : str = digits + 'abcdef' + 'ABCDEF'
octdigits : str = '01234567'
punctuation : str = r"""!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~"""
printable : str = digits + ascii_letters + punctuation + whitespace
