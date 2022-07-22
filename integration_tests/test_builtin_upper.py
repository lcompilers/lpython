from lpython_builtin import (upper)
from ltypes import i32, f64, f32, i64, c32, c64

def test_upper():
    i: str
    i = upper("lpython")
    assert i == "LPYTHON"

test_upper()
