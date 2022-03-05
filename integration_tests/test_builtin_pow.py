from ltypes import i32

def test_pow():
    a: i32
    b: i32
    a = 2
    b = 5
    assert pow(a, b) == 32
    a = 6
    b = 3
    assert pow(a, b) == 216
    a = 2
    b = 0
    assert pow(a, b) == 1
    a = 2
    b = -1
    assert pow(a, b) == 0.5
    a = 6
    b = -4
    assert pow(a, b) == 0.0007716049382716049
    a = -3
    b = -5
    assert pow(a, b) == -0.00411522633744856
    assert pow(0, 0) == 1
    assert pow(6, -4) == 0.0007716049382716049
