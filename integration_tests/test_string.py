from string import (capitalize, upper, lower)
from ltypes import i32, f64, f32, i64, c32, c64

def test_capitalize():
    i: str
    i = capitalize("lpyThon")
    assert i == "Lpython"
    i: str
    i = capitalize("deVeLoPmEnT")
    assert i == "Development"

def test_upper():
    i: str
    i = upper("lpython")
    assert i == "LPYTHON"

def test_lower():
    i: str
    i = lower("CoMpIlEr")
    assert i == "compiler"

def check():
    test_capitalize()
    test_upper()
    test_lower()
check()
