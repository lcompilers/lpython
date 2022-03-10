from ltypes import f32
from numpy import empty, float32

def test_local_arrays():
    a: f32[16]
    a = empty(16, dtype=float32)
    i: i32
    for i in range(16):
        a[i] = i
    assert a[0] == 0
    assert a[1] == 1
    assert a[10] == 10
    assert a[15] == 15

def check():
    test_local_arrays()

check()
