from lpython_builtin import (lower)
from ltypes import i32, f64, f32, i64, c32, c64

def test_lower():
    i: str
    i = lower("CoMpIlEr")
    assert i == "compiler"
test_lower()