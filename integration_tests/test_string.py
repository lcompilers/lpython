from string import (capitalize, upper)
from ltypes import i32, f64, f32, i64, c32, c64

def test_capitalize():
    i: str
    i = capitalize("lpython")
    assert i == "Lpython"
    i: str
    i = capitalize("development")
    assert i == "Development"

def test_upper():
    i: str
    i = upper("lpython")
    assert i == "LPYTHON"


def check():
    test_capitalize()
    test_upper()

check()