from lpython import i32, i64, f64, u16

def test_multiply(a: i32, b: i32) -> i32:
    return a*b

def test_mod(a: i32, b: i32) -> i32:
    return a%b

def main0():
    a: i32
    b: i32
    a = 10
    b = -5
    eps: f64
    eps = 1e-12
    assert test_multiply(a, b) == -50
    i: i64
    i = i64(1)
    i += int(1)
    assert i == i64(2)
    a = 2
    b = 5
    assert test_mod(a, b) == 2
    assert test_mod(23, 3) == 2
    a = 123282374
    b = 32771
    assert test_mod(a, b) == 30643
    a = -5345
    b = -534
    assert a % b == -5
    a = -123282374
    b = 32771
    assert test_mod(a, b) == 2128

    assert 10 | 4 == 14
    assert -105346 | -32771 == -32769
    assert 10 & 4 == 0
    assert -105346 & -32771 == -105348
    assert 10 ^ 4 == 14
    assert -105346 ^ -32771 == 72579
    assert 10 >> 1 == 5
    assert 5 << 1 == 10
    i1: i32 = 10
    i2: i32 = 4
    assert i1 << i2 == 160
    assert i1 >> i2 == 0
    assert i1 & i2 == 0
    assert i1 | i2 == 14
    assert i1 ^ i2 == 14
    assert -i1 ^ -i2 == 10
    i3: i32 = 432534534
    i4: i32 = -4325
    assert i3 | i4 == -225
    assert i4 >> 3 == -541
    assert -i3 & i4 == -432534758
    assert -i3 ^ i4 == 432530657
    a = 10
    a |= 4
    assert a == 14
    a ^= 3
    assert a == 13
    b = 10
    a %= b
    assert a == 3
    b = 4
    a <<= b
    assert a == 48
    a >>= 1
    assert a == 24
    a &= b
    assert a == 0
    b **= 4
    assert b == 256
    # Test Issue 1562
    assert ((-8)%3) == 1
    assert ((8)%-3) == -1
    assert (-8%-3) == -2
    assert abs((11.0%-3.0) - (-1.0)) < eps
    assert abs((-11.0%3.0) - (1.0)) < eps

    # Test issue 1869 and 1870
    a1: u16 = u16(10)
    b1: u16 = u16(3)
    c1: u16 = a1 % b1
    assert c1 == u16(1)
    c1 = a1 // b1
    assert c1 == u16(3)

main0()
