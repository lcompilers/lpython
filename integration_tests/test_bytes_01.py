def f():
    a: bytes = b"This is a test string"
    b: bytes = b"This is another test string"
    c: bytes = b"""Bigger test string with docstrings 
    Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do 
    eiusmod tempor incididunt ut labore et dolore magna aliqua. """


def g(a: bytes) -> bytes:
    return a


def h() -> bytes:
    bar: bytes
    bar = g(b"fiwabcd")
    return b"12jw19\\xq0"


f()
h()
