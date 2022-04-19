from string import (capitalize)
from ltypes import i32, f64, f32, i64, c32, c64

def test_capitalize():
    i: str
    i = capitalize("lpython")
    assert i == "Lpython"
    i: str
    i = capitalize("development")
    assert i == "Development"
