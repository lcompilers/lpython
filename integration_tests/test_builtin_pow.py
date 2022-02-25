from ltypes import i32

def test_pow():
    a: i32
    b: i32
    a, b = 2, 5
    assert pow(a, b) == 32
    a, b = 6, 3
    assert pow(a, b) == 216
    a, b = 2, 0
    assert pow(a, b) == 1
    a, b = 2, -1
    assert pow(a, b) == 0.5
    a, b = 6, -4
    assert pow(a, b) == 0.0007716049382716049
    a, b = -3, -5
    assert pow(a, b) == -0.00411522633744856
    assert pow(0, 0) == 1
    assert pow(6, -4) == 0.0007716049382716049
